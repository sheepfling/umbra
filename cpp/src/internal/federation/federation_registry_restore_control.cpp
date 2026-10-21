#include "internal/federation/federation_registry.hpp"
#include "internal/observability/runtime_instrumentation.hpp"
#include "internal/federation/federation_state_image.hpp"
#include "internal/federation/federation_registry_state_image_validation.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

namespace {

using federation_registry_state_image_validation::isRouteFreeDeliveredApplicationValue;
using federation_registry_state_image_validation::isRouteFreeInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMixedMultipleInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleDirectedInteractionDeclarationImage;
using federation_registry_state_image_validation::isRouteFreeMultipleInteractionPublicationImage;
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
using federation_registry_state_image_validation::isRouteFreeRegionalTsoInteractionImage;
using federation_registry_state_image_validation::isOneQueuedTsoInteractionIdentityRebindingImage;
using federation_registry_state_image_validation::pendingOwnershipAssumptionCallbackCount;

using FederateIdentityMapping = std::map<std::uint64_t, std::uint64_t>;

template <typename LiveMembers>
bool mapSavedFederatesByType(
    FederationStateImage const& image,
    LiveMembers const& liveMembers,
    FederateIdentityMapping& mapping,
    bool& identityChanged) {
  std::map<std::wstring, std::vector<std::uint64_t>> savedIdsByType;
  std::map<std::wstring, std::vector<std::uint64_t>> liveIdsByType;
  std::set<std::uint64_t> savedIds;
  for (FederationStateImageMember const& member : image.members) {
    if (member.id == 0U || !savedIds.insert(member.id).second) {
      return false;
    }
    savedIdsByType[member.type].push_back(member.id);
  }
  for (auto const& [federateId, member] : liveMembers) {
    if (federateId == 0U) {
      return false;
    }
    liveIdsByType[member.type].push_back(federateId);
  }
  if (savedIdsByType.size() != liveIdsByType.size()) {
    return false;
  }
  for (auto& [federateType, savedTypeIds] : savedIdsByType) {
    auto const liveType = liveIdsByType.find(federateType);
    if (liveType == liveIdsByType.end() ||
        liveType->second.size() != savedTypeIds.size()) {
      return false;
    }
    std::sort(savedTypeIds.begin(), savedTypeIds.end());
    auto liveTypeIds = liveType->second;
    std::sort(liveTypeIds.begin(), liveTypeIds.end());
    for (std::size_t index = 0U; index < savedTypeIds.size(); ++index) {
      auto const [position, inserted] = mapping.emplace(
          savedTypeIds[index], liveTypeIds[index]);
      static_cast<void>(position);
      if (!inserted) {
        return false;
      }
      auto const savedMember = std::find_if(
          image.members.begin(),
          image.members.end(),
          [&](FederationStateImageMember const& candidate) {
            return candidate.id == savedTypeIds[index];
          });
      auto const liveMember = liveMembers.find(liveTypeIds[index]);
      if (savedMember == image.members.end() || liveMember == liveMembers.end()) {
        return false;
      }
      identityChanged = identityChanged ||
          savedTypeIds[index] != liveTypeIds[index] ||
          savedMember->name != liveMember->second.name;
    }
  }
  return mapping.size() == image.members.size() &&
      mapping.size() == liveMembers.size();
}

bool isRouteFreeObjectClassPublicationDeclaration(
    FederationStateImageObjectClassAttributeDeclarations const& declarations) {
  if (declarations.federateId == 0U || declarations.subscriptionGeneration != 0U ||
      declarations.classes.size() != 1U) {
    return false;
  }

  FederationStateImageObjectClassAttributeClass const& objectClass =
      declarations.classes.front();
  return objectClass.objectClassHandle != 0U &&
      !objectClass.privilegeToDeleteExplicitlyUnpublished &&
      objectClass.explicitlyPublishedAttributeHandles.size() == 1U &&
      objectClass.explicitlyPublishedAttributeHandles.front() != 0U &&
      objectClass.subscribedAttributes.empty() &&
      objectClass.subscribedUpdateRateDesignators.empty() &&
      objectClass.regionalSubscribedAttributes.empty() &&
      objectClass.regionalSubscribedUpdateRateDesignators.empty() &&
      objectClass.defaultTransportationTypes.empty() &&
      objectClass.defaultOrderTypes.empty();
}

bool isOneOrTwoRouteFreeObjectClassPublicationDeclarationImage(
    FederationStateImage const& image) {
  if (!image.objectClassAttributeDeclarationsPresent ||
      image.objectClassDeclarationCount !=
          image.objectClassAttributeDeclarations.size() ||
      image.objectClassAttributeDeclarations.empty() ||
      image.objectClassAttributeDeclarations.size() > 2U) {
    return false;
  }

  FederationStateImageObjectClassAttributeDeclarations const& firstDeclarations =
      image.objectClassAttributeDeclarations.front();
  if (!isRouteFreeObjectClassPublicationDeclaration(firstDeclarations)) {
    return false;
  }
  FederationStateImageObjectClassAttributeClass const& firstClass =
      firstDeclarations.classes.front();
  std::set<std::uint64_t> declarationFederateIds;
  for (FederationStateImageObjectClassAttributeDeclarations const& declarations :
       image.objectClassAttributeDeclarations) {
    if (!isRouteFreeObjectClassPublicationDeclaration(declarations) ||
        !declarationFederateIds.insert(declarations.federateId).second) {
      return false;
    }
    FederationStateImageObjectClassAttributeClass const& objectClass =
        declarations.classes.front();
    if (objectClass.objectClassHandle != firstClass.objectClassHandle ||
        objectClass.explicitlyPublishedAttributeHandles !=
            firstClass.explicitlyPublishedAttributeHandles) {
      return false;
    }
  }
  return true;
}

bool isOneRouteFreeObjectClassPublicationDeclarationImage(
    FederationStateImage const& image) {
  return image.objectClassAttributeDeclarations.size() == 1U &&
      isOneOrTwoRouteFreeObjectClassPublicationDeclarationImage(image);
}

bool isOneOrTwoRouteFreePublishedObjectApplicationValueImage(
    FederationStateImage const& image,
    bool allowSingleRegionAssociation = false) {
  if (!isOneOrTwoRouteFreeObjectClassPublicationDeclarationImage(image) ||
      image.objectInstanceCount != image.objects.size() ||
      image.objects.empty() || image.objects.size() > 2U) {
    return false;
  }

  FederationStateImageRegion const* associatedRegion = nullptr;
  if (allowSingleRegionAssociation) {
    if (image.objects.size() != 1U || !image.regionsPresent ||
        image.regionCount != 1U || image.regions.size() != 1U) {
      return false;
    }
    associatedRegion = &image.regions.front();
    if (associatedRegion->handle == 0U ||
        associatedRegion->ownerFederateId == 0U ||
        !associatedRegion->inUse || !associatedRegion->specificationCommitted ||
        associatedRegion->dimensionHandles.size() != 1U ||
        associatedRegion->dimensionHandles.front() == 0U ||
        associatedRegion->pendingRangeBounds.size() != 1U ||
        associatedRegion->committedRangeBounds.size() != 1U) {
      return false;
    }
    auto const hasCompleteBounds = [associatedRegion](
                                       FederationStateImageRegionRange const& range) {
      return range.dimensionHandle ==
                 associatedRegion->dimensionHandles.front() &&
          range.lowerBound < range.upperBound;
    };
    if (!hasCompleteBounds(associatedRegion->pendingRangeBounds.front()) ||
        !hasCompleteBounds(associatedRegion->committedRangeBounds.front())) {
      return false;
    }
  } else if (image.regionCount != 0U || !image.regions.empty()) {
    return false;
  }

  FederationStateImageObjectClassAttributeClass const& objectClass =
      image.objectClassAttributeDeclarations.front().classes.front();
  std::map<std::uint64_t, std::size_t> objectCountByProducer;
  std::set<std::uint64_t> objectHandles;
  std::set<std::wstring> objectNames;
  for (FederationStateImageObject const& object : image.objects) {
    auto const declaration = std::find_if(
        image.objectClassAttributeDeclarations.begin(),
        image.objectClassAttributeDeclarations.end(),
        [&object](
            FederationStateImageObjectClassAttributeDeclarations const& candidate) {
          return candidate.federateId == object.producingFederateId;
        });
    if (object.handle == 0U || object.name.empty() ||
        !objectHandles.insert(object.handle).second ||
        !objectNames.insert(object.name).second ||
        object.registeredObjectClassHandle != objectClass.objectClassHandle ||
        declaration == image.objectClassAttributeDeclarations.end() ||
        !isRouteFreeDeliveredApplicationValue(object) ||
        object.attributes.empty() || object.attributeValues.size() != 1U ||
        object.knownObjectClassHandlesByFederate.size() != 1U) {
      return false;
    }
    ++objectCountByProducer[object.producingFederateId];

    FederationStateImageObjectAttributeValue const& value =
        object.attributeValues.front();
    FederationStateImageKnownObjectClass const& knownClass =
        object.knownObjectClassHandlesByFederate.front();
    if (value.attributeHandle !=
            objectClass.explicitlyPublishedAttributeHandles.front() ||
        knownClass.federateId != object.producingFederateId ||
        knownClass.objectClassHandle != object.registeredObjectClassHandle) {
      return false;
    }

    // The class may contribute additional attribute ownership records, but
    // this slice admits only each registering producer's initial ownership.
    // Its object-region extension is limited to the one valued attribute on
    // the single object and the single region validated above.
    std::size_t matchingPublishedAttributeCount = 0U;
    std::size_t associatedRegionReferenceCount = 0U;
    std::set<std::uint64_t> attributeHandles;
    for (FederationStateImageObjectAttribute const& attribute : object.attributes) {
      if (attribute.handle == 0U ||
          !attributeHandles.insert(attribute.handle).second ||
          (!attribute.updateRegionHandles.empty() &&
           (associatedRegion == nullptr ||
            attribute.handle != value.attributeHandle ||
            attribute.updateRegionHandles.size() != 1U ||
            attribute.updateRegionHandles.front() != associatedRegion->handle)) ||
          (attribute.ownerFederateId != 0U &&
           attribute.ownerFederateId != object.producingFederateId)) {
        return false;
      }
      if (!attribute.updateRegionHandles.empty()) {
        ++associatedRegionReferenceCount;
      }
      if (attribute.handle == value.attributeHandle) {
        if (attribute.ownerFederateId != object.producingFederateId) {
          return false;
        }
        ++matchingPublishedAttributeCount;
      }
    }
    if (matchingPublishedAttributeCount != 1U) {
      return false;
    }
    if (associatedRegion != nullptr &&
        (associatedRegion->ownerFederateId != object.producingFederateId ||
         associatedRegionReferenceCount != 1U)) {
      return false;
    }
  }
  for (FederationStateImageObjectClassAttributeDeclarations const& declarations :
       image.objectClassAttributeDeclarations) {
    auto const objectCount = objectCountByProducer.find(declarations.federateId);
    if (objectCount == objectCountByProducer.end() ||
        (image.objectClassAttributeDeclarations.size() == 2U &&
         objectCount->second != 1U)) {
      return false;
    }
  }
  return true;
}

bool isOneRouteFreePublishedObjectApplicationValueWithOneRegionImage(
    FederationStateImage const& image) {
  return isOneOrTwoRouteFreePublishedObjectApplicationValueImage(image, true);
}

bool isOneExplicitUnachievedTwoMemberSynchronizationPointImage(
    FederationStateImage const& image) {
  if (!image.synchronizationPointsPresent ||
      image.synchronizationPointCount != 1U ||
      image.synchronizationPoints.size() != 1U ||
      image.members.size() != 2U) {
    return false;
  }

  std::set<std::uint64_t> savedMemberIds;
  for (FederationStateImageMember const& member : image.members) {
    if (member.id == 0U || !savedMemberIds.insert(member.id).second) {
      return false;
    }
  }

  FederationStateImageSynchronizationPoint const& point =
      image.synchronizationPoints.front();
  auto const namesExactlySavedMembers = [&savedMemberIds](
                                           std::vector<std::uint64_t> const& ids) {
    std::set<std::uint64_t> uniqueIds(ids.begin(), ids.end());
    return ids.size() == savedMemberIds.size() &&
        uniqueIds.size() == ids.size() && uniqueIds == savedMemberIds;
  };
  return !point.label.empty() && !point.lateJoinExpansionAllowed &&
      namesExactlySavedMembers(point.synchronizationSet) &&
      namesExactlySavedMembers(point.announcedFederates) &&
      point.achievedFederates.empty();
}

bool isOnePendingRegularOwnershipIdentityRebindingImage(
    FederationStateImage const& image) {
  // The type-rebinding path admits one durable regular-acquisition request
  // together with the exact publication/subscription graph that made the
  // owner callback possible. The ownership request, not unrelated application
  // ledgers, is the only added transaction state in this image.
  if (image.members.size() != 2U ||
      image.objectInstanceCount != 1U || image.objects.size() != 1U ||
      image.regionCount != 0U || !image.regions.empty() ||
      image.objectClassDeclarationCount !=
          image.objectClassAttributeDeclarations.size() ||
      image.objectClassAttributeDeclarations.size() != 2U ||
      !image.objectClassAttributeDeclarationsPresent) {
    return false;
  }

  std::set<std::uint64_t> savedMemberIds;
  for (FederationStateImageMember const& member : image.members) {
    if (member.id == 0U || !savedMemberIds.insert(member.id).second) {
      return false;
    }
  }

  FederationStateImageObject const& object = image.objects.front();
  if (object.handle == 0U || object.name.empty() ||
      object.registeredObjectClassHandle == 0U ||
      !savedMemberIds.contains(object.producingFederateId) ||
      !isRouteFreePendingRegularOwnershipObject(object) ||
      object.attributes.empty() || object.attributeValues.size() != 1U ||
      object.knownObjectClassHandlesByFederate.size() != savedMemberIds.size() ||
      object.pendingAttributeOwnershipAcquisitionRequests.size() != 1U) {
    return false;
  }

  FederationStateImagePendingAttributeOwnershipAcquisition const& request =
      object.pendingAttributeOwnershipAcquisitionRequests.front();
  if (!savedMemberIds.contains(request.requestingFederateId) ||
      request.requestingFederateId == object.producingFederateId ||
      request.desiredAttributeHandles.size() != 1U ||
      request.releaseCallbacksQueuedByOwningFederate.size() != 1U) {
    return false;
  }
  std::uint64_t const requestedAttributeHandle =
      request.desiredAttributeHandles.front();
  auto const attribute = std::find_if(
      object.attributes.begin(),
      object.attributes.end(),
      [requestedAttributeHandle](FederationStateImageObjectAttribute const& candidate) {
        return candidate.handle == requestedAttributeHandle;
      });
  if (attribute == object.attributes.end()) {
    return false;
  }
  auto const& firstDeclarations =
      image.objectClassAttributeDeclarations.front();
  if (firstDeclarations.classes.size() != 1U) {
    return false;
  }
  FederationStateImageObjectClassAttributeClass const& firstObjectClass =
      firstDeclarations.classes.front();
  if (firstObjectClass.objectClassHandle == 0U ||
      firstObjectClass.explicitlyPublishedAttributeHandles !=
          std::vector<std::uint64_t>{requestedAttributeHandle}) {
    return false;
  }
  std::uint64_t const objectClassHandle = firstObjectClass.objectClassHandle;

  std::set<std::uint64_t> declarationFederateIds;
  bool requesterHasExactAttributeSubscription = false;
  for (FederationStateImageObjectClassAttributeDeclarations const& declarations :
       image.objectClassAttributeDeclarations) {
    if (!savedMemberIds.contains(declarations.federateId) ||
        !declarationFederateIds.insert(declarations.federateId).second ||
        declarations.classes.size() != 1U) {
      return false;
    }
    FederationStateImageObjectClassAttributeClass const& memberClass =
        declarations.classes.front();
    if (memberClass.objectClassHandle != objectClassHandle ||
        memberClass.privilegeToDeleteExplicitlyUnpublished ||
        memberClass.explicitlyPublishedAttributeHandles !=
            std::vector<std::uint64_t>{requestedAttributeHandle} ||
        !memberClass.regionalSubscribedAttributes.empty() ||
        !memberClass.regionalSubscribedUpdateRateDesignators.empty() ||
        !memberClass.defaultTransportationTypes.empty() ||
        !memberClass.defaultOrderTypes.empty()) {
      return false;
    }
    if (declarations.federateId == request.requestingFederateId) {
      if (memberClass.subscribedAttributes.size() != 1U ||
          memberClass.subscribedAttributes.front().attributeHandle !=
              requestedAttributeHandle ||
          !memberClass.subscribedAttributes.front().active ||
          memberClass.subscribedUpdateRateDesignators.size() != 1U ||
          memberClass.subscribedUpdateRateDesignators.front().attributeHandle !=
              requestedAttributeHandle) {
        return false;
      }
      requesterHasExactAttributeSubscription = true;
    } else if (declarations.federateId == object.producingFederateId) {
      if (!memberClass.subscribedAttributes.empty() ||
          !memberClass.subscribedUpdateRateDesignators.empty()) {
        return false;
      }
    } else {
      return false;
    }
  }
  if (declarationFederateIds != savedMemberIds ||
      !requesterHasExactAttributeSubscription) {
    return false;
  }

  std::set<std::uint64_t> objectAttributeHandles;
  bool producerOwnsRequestedAttribute = false;
  for (FederationStateImageObjectAttribute const& candidate : object.attributes) {
    if (candidate.handle == 0U ||
        !objectAttributeHandles.insert(candidate.handle).second ||
        !candidate.updateRegionHandles.empty() ||
        (candidate.ownerFederateId != 0U &&
         candidate.ownerFederateId != object.producingFederateId)) {
      return false;
    }
    if (candidate.handle == requestedAttributeHandle) {
      if (candidate.ownerFederateId != object.producingFederateId) {
        return false;
      }
      producerOwnsRequestedAttribute = true;
    }
  }
  std::set<std::uint64_t> knownFederateIds;
  for (FederationStateImageKnownObjectClass const& knownClass :
       object.knownObjectClassHandlesByFederate) {
    if (!savedMemberIds.contains(knownClass.federateId) ||
        !knownFederateIds.insert(knownClass.federateId).second ||
        knownClass.objectClassHandle != object.registeredObjectClassHandle) {
      return false;
    }
  }
  if (!producerOwnsRequestedAttribute || knownFederateIds != savedMemberIds) {
    return false;
  }
  FederationStateImageObjectAttributeValue const& value =
      object.attributeValues.front();
  auto const& [ownerFederateId, releasedAttributes] =
      request.releaseCallbacksQueuedByOwningFederate.front();
  if (object.registeredObjectClassHandle != objectClassHandle ||
      value.attributeHandle != requestedAttributeHandle ||
      ownerFederateId != object.producingFederateId ||
      releasedAttributes != request.desiredAttributeHandles) {
    return false;
  }
  return true;
}

bool hasOnlySupportedRebindableRestoreLedgers(
    FederationStateImage const& image) {
  bool const hasNoInteractionDeclarations =
      image.interactionDeclarationCount == 0U &&
      image.interactionDeclarations.empty();
  bool const hasOneRouteFreeInteractionDeclaration =
      isRouteFreeInteractionDeclarationImage(image);
  bool const hasNoObjectClassDeclarations =
      image.objectClassDeclarationCount == 0U &&
      image.objectClassAttributeDeclarations.empty();
  bool const hasOneObjectClassPublicationDeclaration =
      isOneRouteFreeObjectClassPublicationDeclarationImage(image);
  bool const hasNoObjects =
      image.objectInstanceCount == 0U && image.objects.empty();
  bool const hasOneOrTwoPublishedObjectApplicationValues =
      isOneOrTwoRouteFreePublishedObjectApplicationValueImage(image);
  bool const hasOnePublishedObjectApplicationValueWithOneRegion =
      isOneRouteFreePublishedObjectApplicationValueWithOneRegionImage(image);
  bool const hasNoSynchronizationPoints =
      image.synchronizationPointCount == 0U &&
      image.synchronizationPoints.empty();
  bool const hasOneStandaloneSynchronizationPoint =
      isOneExplicitUnachievedTwoMemberSynchronizationPointImage(image) &&
      hasNoInteractionDeclarations && hasNoObjectClassDeclarations && hasNoObjects;
  bool const hasOnePendingRegularOwnershipLedger =
      isOnePendingRegularOwnershipIdentityRebindingImage(image) &&
      hasNoInteractionDeclarations &&
      image.synchronizationPointCount == 0U &&
      image.synchronizationPoints.empty();
  bool const hasOneQueuedTsoInteractionLedger =
      isOneQueuedTsoInteractionIdentityRebindingImage(image);
  bool const hasOneAllowedDeclarationLedger =
      (hasNoInteractionDeclarations && hasNoObjectClassDeclarations && hasNoObjects) ||
      (hasOneRouteFreeInteractionDeclaration && hasNoObjectClassDeclarations && hasNoObjects) ||
      (hasNoInteractionDeclarations && hasOneObjectClassPublicationDeclaration && hasNoObjects) ||
      (hasNoInteractionDeclarations &&
       (hasOneOrTwoPublishedObjectApplicationValues ||
        hasOnePublishedObjectApplicationValueWithOneRegion));
  bool const hasNoTsoPayloadLedgers =
      image.tsoInteractionMessageCount == 0U &&
      image.tsoAttributeUpdateMessageCount == 0U &&
      image.tsoObjectDeletionMessageCount == 0U &&
      image.tsoDirectedInteractionMessageCount == 0U &&
      image.tsoInteractionMessages.empty() &&
      image.tsoAttributeUpdateMessages.empty() &&
      image.tsoObjectDeletionMessages.empty() &&
      image.tsoRequestRetractionRecords.empty() &&
      image.tsoDirectedInteractionMessages.empty() &&
      image.tsoQueueEntries.empty();
  bool const hasOneNonTsoRebindableLedger =
      ((hasOneAllowedDeclarationLedger && hasNoSynchronizationPoints) ||
       hasOneStandaloneSynchronizationPoint ||
       hasOnePendingRegularOwnershipLedger) &&
      image.regionCount ==
          (hasOnePublishedObjectApplicationValueWithOneRegion ? 1U : 0U) &&
      hasNoTsoPayloadLedgers &&
      image.deferredUpdateRegionAssociations.empty() &&
      image.pendingAttributeOwnershipQueries.empty() &&
      image.pendingAttributeOwnershipAssumptions.empty() &&
      image.queuedReceiveOrderInteractions.empty() &&
      image.queuedReceiveOrderAttributeUpdates.empty() &&
      image.reservedObjectInstanceNames.empty() &&
      (hasOnePublishedObjectApplicationValueWithOneRegion
           ? image.regions.size() == 1U
           : image.regions.empty());
  return hasOneNonTsoRebindableLedger || hasOneQueuedTsoInteractionLedger;
}

template <typename LiveMembers>
bool rebindSupportedFederateIdentityLedgers(
    FederationStateImage& image,
    FederateIdentityMapping const& mapping,
    LiveMembers const& liveMembers) {
  bool const hasOneQueuedTsoIdentityRebindingLedger =
      isOneQueuedTsoInteractionIdentityRebindingImage(image);
  // The object producer and publication declaration refer to the same saved
  // member. Validate that cross-linked graph before either side is rebound.
  bool const hasOneOrTwoPublishedObjectApplicationValues =
      image.objects.empty() ||
      isOneOrTwoRouteFreePublishedObjectApplicationValueImage(image) ||
      isOneRouteFreePublishedObjectApplicationValueWithOneRegionImage(image) ||
      isOnePendingRegularOwnershipIdentityRebindingImage(image);
  if (!hasOneOrTwoPublishedObjectApplicationValues) {
    return false;
  }
  if (!(image.synchronizationPointCount == 0U &&
        image.synchronizationPoints.empty()) &&
      !isOneExplicitUnachievedTwoMemberSynchronizationPointImage(image)) {
    return false;
  }

  for (FederationStateImageMember& member : image.members) {
    auto const reboundId = mapping.find(member.id);
    if (reboundId == mapping.end()) {
      return false;
    }
    auto const liveMember = liveMembers.find(reboundId->second);
    if (liveMember == liveMembers.end() || liveMember->second.type != member.type) {
      return false;
    }
    member.id = reboundId->second;
    member.name = liveMember->second.name;
  }
  for (FederationStateImageTimeState& timeState : image.timeStates) {
    auto const reboundId = mapping.find(timeState.federateId);
    if (reboundId == mapping.end()) {
      return false;
    }
    timeState.federateId = reboundId->second;
  }
  if (!image.interactionDeclarations.empty()) {
    if (!isRouteFreeInteractionDeclarationImage(image) &&
        !hasOneQueuedTsoIdentityRebindingLedger) {
      return false;
    }
    for (FederationStateImageInteractionDeclaration& declaration :
         image.interactionDeclarations) {
      auto const reboundId = mapping.find(declaration.federateId);
      if (reboundId == mapping.end()) {
        return false;
      }
      declaration.federateId = reboundId->second;
    }
  }
  if (!image.objectClassAttributeDeclarations.empty()) {
    // A no-object image may contain only the standalone one-declaration
    // publication slice. When objects are present, the linked one/two-
    // producer application graph was prevalidated before any identity edits.
    if (image.objects.empty() &&
        !isOneRouteFreeObjectClassPublicationDeclarationImage(image)) {
      return false;
    }
    for (FederationStateImageObjectClassAttributeDeclarations& declarations :
         image.objectClassAttributeDeclarations) {
      auto const reboundId = mapping.find(declarations.federateId);
      if (reboundId == mapping.end()) {
        return false;
      }
      declarations.federateId = reboundId->second;
    }
  }
  for (FederationStateImageRegion& region : image.regions) {
    auto const reboundOwner = mapping.find(region.ownerFederateId);
    if (reboundOwner == mapping.end()) {
      return false;
    }
    region.ownerFederateId = reboundOwner->second;
  }
  for (FederationStateImageSynchronizationPoint& point :
       image.synchronizationPoints) {
    auto rebindParticipantIds = [&mapping](
                                    std::vector<std::uint64_t>& ids) {
      for (std::uint64_t& id : ids) {
        auto const reboundId = mapping.find(id);
        if (reboundId == mapping.end()) {
          return false;
        }
        id = reboundId->second;
      }
      return true;
    };
    if (!rebindParticipantIds(point.synchronizationSet) ||
        !rebindParticipantIds(point.announcedFederates)) {
      return false;
    }
  }
  if (!image.objects.empty()) {
    for (FederationStateImageObject& object : image.objects) {
      auto const reboundProducer = mapping.find(object.producingFederateId);
      if (reboundProducer == mapping.end()) {
        return false;
      }
      object.producingFederateId = reboundProducer->second;
      for (FederationStateImageObjectAttribute& attribute : object.attributes) {
        if (attribute.ownerFederateId == 0U) {
          continue;
        }
        auto const reboundOwner = mapping.find(attribute.ownerFederateId);
        if (reboundOwner == mapping.end()) {
          return false;
        }
        attribute.ownerFederateId = reboundOwner->second;
      }
      for (FederationStateImageKnownObjectClass& knownClass :
           object.knownObjectClassHandlesByFederate) {
        auto const reboundKnownFederate = mapping.find(knownClass.federateId);
        if (reboundKnownFederate == mapping.end()) {
          return false;
        }
        knownClass.federateId = reboundKnownFederate->second;
      }
      for (FederationStateImagePendingAttributeOwnershipAcquisition& request :
           object.pendingAttributeOwnershipAcquisitionRequests) {
        auto const reboundRequester = mapping.find(request.requestingFederateId);
        if (reboundRequester == mapping.end()) {
          return false;
        }
        request.requestingFederateId = reboundRequester->second;
        for (auto& [ownerFederateId, attributes] :
             request.releaseCallbacksQueuedByOwningFederate) {
          static_cast<void>(attributes);
          auto const reboundOwner = mapping.find(ownerFederateId);
          if (reboundOwner == mapping.end()) {
            return false;
          }
          ownerFederateId = reboundOwner->second;
        }
      }
    }
  }
  for (FederationStateImageTsoInteractionMessage& message :
       image.tsoInteractionMessages) {
    auto const reboundProducer = mapping.find(message.producingFederateId);
    if (reboundProducer == mapping.end()) {
      return false;
    }
    message.producingFederateId = reboundProducer->second;
  }
  for (FederationStateImageTsoRequestRetractionRecord& record :
       image.tsoRequestRetractionRecords) {
    auto const reboundProducer = mapping.find(record.producingFederateId);
    if (reboundProducer == mapping.end()) {
      return false;
    }
    record.producingFederateId = reboundProducer->second;
    for (FederationStateImageTsoRequestRetractionRecipient& recipient :
         record.recipientStates) {
      auto const reboundRecipient = mapping.find(recipient.receivingFederateId);
      if (reboundRecipient == mapping.end()) {
        return false;
      }
      recipient.receivingFederateId = reboundRecipient->second;
    }
  }
  for (FederationStateImageTsoQueueEntry& entry : image.tsoQueueEntries) {
    auto const reboundRecipient = mapping.find(entry.recipientFederateId);
    if (reboundRecipient == mapping.end()) {
      return false;
    }
    entry.recipientFederateId = reboundRecipient->second;
  }

  std::map<std::uint64_t, std::wstring> reboundNames;
  for (auto const& mappingEntry : mapping) {
    auto const liveMember = liveMembers.find(mappingEntry.second);
    if (liveMember == liveMembers.end()) {
      return false;
    }
    reboundNames.emplace(mappingEntry.second, liveMember->second.name);
  }
  for (auto const& [savedId, savedName] : image.federateNamesById) {
    if (!mapping.contains(savedId)) {
      reboundNames.try_emplace(savedId, savedName);
    }
  }
  image.federateNamesById.assign(reboundNames.begin(), reboundNames.end());
  return true;
}

}  // namespace

FederationRestoreControlResult EmbeddedFederationRegistry::requestFederationRestore(
    std::wstring const& federationName,
    std::uint64_t requestingFederateId,
    std::wstring label) {
  auto instrumentationScope = beginInstrumentation("requestFederationRestore");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  auto const requester = federation->second.members.find(requestingFederateId);
  if (requester == federation->second.members.end()) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_in_progress;
    return result;
  }

  auto const savedFederations = saveSnapshots_.find(federationName);
  auto const saved = savedFederations == saveSnapshots_.end()
      ? std::map<std::wstring, Federation>::const_iterator{}
      : savedFederations->second.find(label);
  bool const processLocalSnapshot =
      savedFederations != saveSnapshots_.end() && saved != savedFederations->second.end();

  auto const requesterRoute = federation->second.interactionCallbackRoutes.find(
      requestingFederateId);
  if (requesterRoute == federation->second.interactionCallbackRoutes.end() ||
      !requesterRoute->second) {
    result.status = FederationRestoreControlStatus::callback_route_missing;
    return result;
  }
  auto const requesterReportRoute = federation->second.serviceReportRoutes.find(
      requestingFederateId);
  auto const selectedRequesterReportRoute =
      requesterReportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : requesterReportRoute->second;
  auto const requesterPublicReportRoute =
      federation->second.publicServiceReportRoutes.find(requestingFederateId);
  auto const selectedRequesterPublicReportRoute =
      requesterPublicReportRoute == federation->second.publicServiceReportRoutes.end()
      ? FederatePublicServiceReportRoute{}
      : requesterPublicReportRoute->second;

  // A process-local snapshot remains the richest restore source. If it is
  // absent (for example after a registry restart), the filesystem commit is
  // still eligible for the bounded control/temporal materializer below.
  Federation const& snapshot = processLocalSnapshot ? saved->second : federation->second;
  // Restore is admitted only when the durable commit envelope still agrees
  // with the in-process image.  The envelope is intentionally not treated as
  // a complete state image yet, but a missing/corrupt/mismatched record must
  // never be silently ignored by a future disk-backed restore path.
  bool durableCommitMatches = false;
  bool membershipTypeCountsMatch = false;
  bool restoreFromProcessLocalSnapshot = processLocalSnapshot;
  std::map<std::uint64_t, std::wstring> savedFederateNamesByLiveId;
  std::optional<FederationStateImage> durableStateImage;
  try {
    auto const durableCommit = saveCommitStore_->load(federationName, label);
    if (durableCommit && durableCommit->federationName == federationName &&
        durableCommit->label == label &&
         durableCommit->logicalTimeImplementationName ==
             snapshot.definition.logicalTimeImplementationName) {
      durableCommitMatches = true;
      if (processLocalSnapshot) {
        if (durableCommit->memberFederateIds.size() != snapshot.members.size()) {
          durableCommitMatches = false;
        }
        for (auto const& [memberId, member] : snapshot.members) {
          static_cast<void>(member);
          if (std::find(
                  durableCommit->memberFederateIds.begin(),
                  durableCommit->memberFederateIds.end(),
                  memberId) == durableCommit->memberFederateIds.end()) {
            durableCommitMatches = false;
            break;
          }
        }
      }
      if (durableCommitMatches && !durableCommit->stateImage.empty()) {
        try {
          auto image = FederationStateImageCodec::decode(
              durableCommit->stateImage);
          if (image.federationName != federationName ||
              image.logicalTimeImplementationName !=
                  snapshot.definition.logicalTimeImplementationName ||
              image.members.size() != durableCommit->memberFederateIds.size() ||
              (processLocalSnapshot &&
               image.members.size() != snapshot.members.size())) {
            durableCommitMatches = false;
          }
          if (processLocalSnapshot && durableCommitMatches &&
              image.normalizationSeed != snapshot.normalizationSeed) {
            durableCommitMatches = false;
          }
          if (durableCommitMatches) {
            std::set<std::uint64_t> imageMemberIds;
            for (FederationStateImageMember const& imageMember : image.members) {
              if (!imageMemberIds.insert(imageMember.id).second ||
                  std::find(
                      durableCommit->memberFederateIds.begin(),
                      durableCommit->memberFederateIds.end(),
                      imageMember.id) == durableCommit->memberFederateIds.end()) {
                durableCommitMatches = false;
                break;
              }
              if (processLocalSnapshot) {
                auto const snapshotMember = snapshot.members.find(imageMember.id);
                if (snapshotMember == snapshot.members.end() ||
                    snapshotMember->second.name != imageMember.name ||
                    snapshotMember->second.type != imageMember.type) {
                  durableCommitMatches = false;
                  break;
                }
              }
            }
            if (durableCommitMatches &&
                imageMemberIds.size() != durableCommit->memberFederateIds.size()) {
              durableCommitMatches = false;
            }
          }
          if (durableCommitMatches) {
            if (processLocalSnapshot) {
              // Canonical re-encoding compares every v1 field (switches,
              // section counts, allocator floors, member control state, and
              // official encoded temporal values), not just the identity fence
              // above. A syntactically valid but stale/tampered payload must not
              // be accepted as the image for this in-process snapshot.
              auto const expectedImage = FederationStateImageCodec::encode(
                  stateImageFor(snapshot, federationName));
              durableCommitMatches =
                  FederationStateImageCodec::encode(image) == expectedImage;
            } else {
              // The restart path materializes the scalar/member/time image,
              // route-free declaration ledgers, the bounded object
              // application-value projection, the bounded regular, If
              // Available, negotiated regular, negotiated If Available, and
              // mixed negotiated and confirmation-delivered regular/If Available
              // ownership ledgers,
              // and the ordinary timestamped
              // interaction, directed-interaction, and attribute-update
              // payload/queue ledgers. A timestamped object deletion is also
              // admitted when its invocation snapshot is explicitly bounded
              // to the object value ledger plus the pending deletion marker;
              // other callback-bearing application ledgers still require a
              // live object snapshot until their route-free images have an
              // explicit restart contract.
              auto const hasNoObjectCallbackLedgers =
                  [&image](FederationStateImageObject const& object) {
                    return object.pendingAttributeOwnershipAcquisitionIfAvailableRequests.empty() &&
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
                        pendingOwnershipAssumptionCallbackCount(image, object.handle) == 0U &&
                        object.pendingDiscoveryFederateIds.empty() &&
                        object.pendingRemovalFederateIds.empty() &&
                        object.connectionLossAutomaticRemovalFederateIds.empty() &&
                        object.deferredConnectionLossTsoRemovalFederateIds.empty() &&
                        std::all_of(
                            object.attributes.begin(),
                            object.attributes.end(),
                            [](FederationStateImageObjectAttribute const& attribute) {
                              return attribute.updateRegionHandles.empty();
                            });
                  };
              auto const isRouteFreeObjectApplicationValue =
                  [&](FederationStateImageObject const& object) {
                    return object.attributeValuesPresent &&
                        !object.attributeValues.empty() &&
                        !object.deleteAccepted &&
                        object.pendingOperationCount == 0U &&
                        hasNoObjectCallbackLedgers(object) &&
                        object.pendingTimestampedRemovalFederateIds.empty() &&
                        !object.pendingTimestampedDeletionMessageId.has_value();
                  };
              auto const hasMatchingDeletionPayload =
                  [&](FederationStateImageObject const& object) {
                    return object.pendingTimestampedDeletionMessageId.has_value() &&
                        std::any_of(
                            image.tsoObjectDeletionMessages.begin(),
                            image.tsoObjectDeletionMessages.end(),
                            [&](FederationStateImageTsoObjectDeletionMessage const& message) {
                              return message.messageId ==
                                      *object.pendingTimestampedDeletionMessageId &&
                                  message.objectInstanceHandle == object.handle &&
                                  message.producingFederateId == object.producingFederateId &&
                                  message.reconstitution.has_value();
                            });
                  };
              auto const isRouteFreeObjectDeletionInvocation =
                  [&](FederationStateImageObject const& object) {
                    return object.attributeValuesPresent &&
                        !object.attributeValues.empty() &&
                        !object.deleteAccepted &&
                        !object.pendingTimestampedRemovalFederateIds.empty() &&
                        object.pendingTimestampedDeletionMessageId.has_value() &&
                        object.pendingOperationCount ==
                            object.pendingTimestampedRemovalFederateIds.size() + 1U &&
                        hasNoObjectCallbackLedgers(object) &&
                        hasMatchingDeletionPayload(object);
                  };
              bool const restartableObjectLedger =
                  !image.objects.empty() &&
                  std::all_of(
                      image.objects.begin(),
                      image.objects.end(),
                      [&](FederationStateImageObject const& object) {
                        if (!object.pendingAttributeValueUpdateRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateObject(object);
                        }
                        if (!object.pendingAttributeValueUpdateClassRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateClassObject(object);
                        }
                        if (!object.pendingAttributeValueUpdateRegionalRequests.empty()) {
                          return isRouteFreePendingAttributeValueUpdateRegionalObject(object);
                        }
                        if (isRouteFreePendingAttributeOwnershipQueryObject(object, image)) {
                          return true;
                        }
                        if (isRouteFreePendingOwnershipAssumptionObject(object, image)) {
                          return true;
                        }
                            return isRouteFreeDeliveredApplicationValue(object) ||
                            isRouteFreeObjectApplicationValue(object) ||
                            isRouteFreeObjectWithTsoAttributeUpdate(object, image) ||
                            isRouteFreeObjectDeletionInvocation(object) ||
                            isRouteFreePendingRegularOwnershipObject(object) ||
                            isRouteFreePendingIfAvailableOwnershipObject(object) ||
                            isRouteFreePendingNegotiatedOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedIfAvailableOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingNegotiatedIfAvailableConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedConfirmationDeliveredOwnershipObject(object, image) ||
                            isRouteFreePendingMixedNegotiatedConfirmationAsymmetricOwnershipObject(object, image) ||
                            isRouteFreePendingDivestitureIfWantedNotificationObject(object) ||
                            isRouteFreePendingConfirmDivestitureNotificationObject(object) ||
                            isRouteFreePendingConfirmDivestitureWithOwnershipAssumptionObject(
                                object,
                                image) ||
                            isRouteFreePendingOwnershipAcquisitionCancellationObject(object) ||
                            isRouteFreePendingAttributeTransportationTypeChangeObject(object);
                      });
              bool const restartableTsoPayloadLedger =
                  std::all_of(
                      image.tsoObjectDeletionMessages.begin(),
                      image.tsoObjectDeletionMessages.end(),
                      [](FederationStateImageTsoObjectDeletionMessage const& message) {
                        return message.reconstitution.has_value();
                      });
              bool const restartableInteractionLedger =
                  image.interactionDeclarations.empty() ||
                  isRouteFreeInteractionDeclarationImage(image) ||
                  isOneQueuedTsoInteractionIdentityRebindingImage(image) ||
                  isRouteFreeMultipleInteractionPublicationImage(image) ||
                  isRouteFreeMixedMultipleInteractionDeclarationImage(image) ||
                  isRouteFreeMultipleDirectedInteractionDeclarationImage(image) ||
                  isRouteFreeRegionalTsoInteractionImage(image);
              durableCommitMatches =
                  restartableInteractionLedger &&
                  (image.objects.empty()
                       ? image.tsoObjectDeletionMessages.empty()
                       : restartableObjectLedger) &&
                  restartableTsoPayloadLedger;
            }
            if (durableCommitMatches) {
              FederateIdentityMapping identityMapping;
              bool identityChanged = false;
              membershipTypeCountsMatch = mapSavedFederatesByType(
                  image,
                  federation->second.members,
                  identityMapping,
                  identityChanged);
              if (membershipTypeCountsMatch) {
                for (FederationStateImageMember const& imageMember : image.members) {
                  auto const liveId = identityMapping.find(imageMember.id);
                  if (liveId == identityMapping.end() ||
                      !savedFederateNamesByLiveId.emplace(
                          liveId->second,
                          imageMember.name).second) {
                    durableCommitMatches = false;
                    break;
                  }
                }
              }
              if (durableCommitMatches && identityChanged) {
                // This bounded slice admits one route-free interaction
                // declaration, one object-class publication declaration, the
                // matching one-or-two-object application-value graph, one
                // standalone explicit unachieved synchronization point, one
                // pending regular ownership transaction, or one queued TSO
                // interaction with its exact publisher/subscriber, queue, and
                // retraction ledgers. Every admitted member reference is
                // rewritten through the same type-preserving map; combining
                // these transaction ledgers remains outside this slice.
                bool const supported = hasOnlySupportedRebindableRestoreLedgers(image);
                bool const rebound = supported && rebindSupportedFederateIdentityLedgers(
                    image,
                    identityMapping,
                    federation->second.members);
                if (!supported || !rebound) {
                  durableCommitMatches = false;
                } else {
                  restoreFromProcessLocalSnapshot = false;
                }
              }
            }
            if (durableCommitMatches) {
              durableStateImage = std::move(image);
            }
          }
        } catch (...) {
          durableCommitMatches = false;
        }
      }
    }
  } catch (...) {
    durableCommitMatches = false;
  }
  if (!durableCommitMatches) {
    result.status = FederationRestoreControlStatus::snapshot_not_found;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    notification.publicServiceReportRoute = selectedRequesterPublicReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }
  bool const membershipMatches = membershipTypeCountsMatch;
  if (!membershipMatches) {
    result.status = FederationRestoreControlStatus::membership_mismatch;
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::request_failed;
    notification.receivingFederateId = requestingFederateId;
    notification.label = label;
    notification.callbackRoute = requesterRoute->second;
    notification.serviceReportRoute = selectedRequesterReportRoute;
    notification.publicServiceReportRoute = selectedRequesterPublicReportRoute;
    result.notifications.push_back(std::move(notification));
    return result;
  }

  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      result.status = FederationRestoreControlStatus::callback_route_missing;
      return result;
    }
  }

  Federation::RestoreOperation operation;
  operation.label = label;
  operation.stateImage = std::move(durableStateImage);
  operation.processLocalSnapshot = restoreFromProcessLocalSnapshot;
  for (auto const& [federateId, member] : federation->second.members) {
    static_cast<void>(member);
    operation.statuses.emplace(federateId, rti1516_2025::FEDERATE_RESTORING);
  }
  federation->second.restoreOperation = std::move(operation);

  FederationRestoreNotification succeeded;
  succeeded.kind = FederationRestoreNotificationKind::request_succeeded;
  succeeded.receivingFederateId = requestingFederateId;
  succeeded.label = label;
  succeeded.callbackRoute = requesterRoute->second;
  succeeded.serviceReportRoute = selectedRequesterReportRoute;
  succeeded.publicServiceReportRoute = selectedRequesterPublicReportRoute;
  result.notifications.push_back(std::move(succeeded));

  // The embedded backend begins the restore immediately after accepting the
  // request. The official callbacks still preserve the standard ordering:
  // request success, federation-begun, then one initiate callback per member.
  for (auto const& [federateId, member] : federation->second.members) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
    FederationRestoreNotification begun;
    begun.kind = FederationRestoreNotificationKind::begin;
    begun.receivingFederateId = federateId;
    begun.label = label;
    begun.callbackRoute = route->second;
    begun.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    begun.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(begun));

    FederationRestoreNotification initiate;
    initiate.kind = FederationRestoreNotificationKind::initiate;
    initiate.receivingFederateId = federateId;
    initiate.label = label;
    initiate.federateName = savedFederateNamesByLiveId.at(federateId);
    initiate.preRestoreFederateId = federateId;
    initiate.postRestoreFederateId = federateId;
    initiate.callbackRoute = route->second;
    initiate.serviceReportRoute =
        reportRoute == federation->second.serviceReportRoutes.end()
        ? FederateServiceReportRoute{}
        : reportRoute->second;
    initiate.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(initiate));
  }
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::federateRestoreComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateRestoreComplete");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  auto status = federation->second.restoreOperation->statuses.find(federateId);
  if (status == federation->second.restoreOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_RESTORING) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  status->second = rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE;

  bool allComplete = true;
  for (auto const& [participantId, participantStatus] :
       federation->second.restoreOperation->statuses) {
    static_cast<void>(participantId);
    if (participantStatus != rti1516_2025::FEDERATE_WAITING_FOR_FEDERATION_TO_RESTORE) {
      allComplete = false;
      break;
    }
  }
  if (!allComplete) {
    return result;
  }

  auto const label = federation->second.restoreOperation->label;
  bool const processLocalSnapshot =
      federation->second.restoreOperation->processLocalSnapshot;
  auto const savedFederations = saveSnapshots_.find(federationName);
  auto const saved = savedFederations == saveSnapshots_.end()
      ? std::map<std::wstring, Federation>::const_iterator{}
      : savedFederations->second.find(label);
  if (processLocalSnapshot &&
      (savedFederations == saveSnapshots_.end() ||
       saved == savedFederations->second.end())) {
    result.status = FederationRestoreControlStatus::restore_failed;
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    return result;
  }

  try {
    auto const& restoreOperation = federation->second.restoreOperation;
    if (!restoreOperation || !restoreOperation->stateImage) {
      throw std::logic_error(
          "A successful restore request has no validated durable state image.");
    }
    // A process-local save carries the complete private snapshot. A
    // restarted registry instead starts from its current live membership and
    // applies the route-free control/temporal image before the common typed
    // rehydration helpers run.
    Federation snapshot = processLocalSnapshot ? saved->second : federation->second;
    if (!processLocalSnapshot) {
      restoreControlAndTimeFromStateImage(
          snapshot,
          *restoreOperation->stateImage,
          federation->second);
    }
    restoreMemberInteractionReceiptTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectInstanceNameReservationsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreRegionsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreSynchronizationPointsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectClassAttributeDeclarationsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    if (!processLocalSnapshot) {
      restoreInteractionDeclarationsFromStateImage(
          snapshot,
          *restoreOperation->stateImage,
          federation->second);
    }
    restoreMemberInteractionSendTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberObjectLifecycleTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberReflectionTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreMemberUpdateTelemetryFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreObjectOwnershipLedgersFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second,
        !processLocalSnapshot);
    restoreTsoPayloadsFromStateImage(
        snapshot,
        *restoreOperation->stateImage,
        federation->second);
    restoreTsoQueueFromStateImage(snapshot, *restoreOperation->stateImage);
    snapshot.queuedReceiveOrderInteractions =
        restoreOperation->stateImage->queuedReceiveOrderInteractions;
    snapshot.queuedReceiveOrderAttributeUpdates =
        restoreOperation->stateImage->queuedReceiveOrderAttributeUpdates;
    restoreFederationFromSnapshot(federation->second, snapshot);
    if (!processLocalSnapshot) {
      // A durable image carries declaration state, while the relevance sets
      // are derived transition baselines. Seed them after the image is
      // applied so the next declaration mutation reports only a real edge;
      // the synthetic planner output is intentionally discarded here.
      auto declarationAdvisories =
          planDeclarationAdvisoriesLocked(federationName);
      static_cast<void>(declarationAdvisories);

      // The durable image retains regular acquisition requests but not the
      // callback closures from the process that created it. Re-plan each
      // request now that the fresh registry owns the live membership routes.
      for (auto& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        static_cast<void>(objectInstanceHandle);
        auto workItems = planPendingAttributeOwnershipAcquisitionWork(
            federation->second,
            objectInstance);
        result.ownershipAcquisitionWorkItems.insert(
            result.ownershipAcquisitionWorkItems.end(),
            std::make_move_iterator(workItems.begin()),
            std::make_move_iterator(workItems.end()));
      }
      // An If Available request is an accepted requester callback, not an
      // owner-side release reservation.  Rebind one callback work item per
      // restored request to the current requester route; the callback will
      // re-evaluate ownership at its normal begin boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
          // A negotiated divestiture that selected this If Available request
          // owns the next callback boundary: replaying the ordinary WTA
          // notification as well would consume the request as unavailable
          // before the rebuilt Request Divestiture Confirmation can transfer
          // ownership.  The bounded restart predicate admits only requests
          // whose complete desired set is covered by this negotiated ledger.
          bool negotiatedIfAvailableCandidate = false;
          for (auto const& [attributeHandle, divestiture] :
               objectInstance.pendingNegotiatedAttributeOwnershipDivestitures) {
            if (divestiture.acquiringFederateIsIfAvailable &&
                divestiture.acquiringFederateId == request.requestingFederateId &&
                divestiture.acquisitionRequestId == requestId &&
                request.desiredAttributeHandles.contains(attributeHandle)) {
              negotiatedIfAvailableCandidate = true;
              break;
            }
          }
          if (negotiatedIfAvailableCandidate) {
            continue;
          }
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored If Available ownership reservation has no live callback route.");
          }
          result.ownershipAcquisitionWorkItems.push_back({
              AttributeOwnershipAcquisitionWorkKind::if_available_notification,
              request.requestingFederateId,
              request.requestingFederateId,
              objectInstanceHandle,
              requestId,
              {},
              request.userSuppliedTag,
              true,
              callbackRoute->second,
          });
        }
      }
      // A cancellation reservation is a separate requester callback from the
      // regular acquisition work-item family. Rebind each durable reservation
      // to the live requester route after restore; the callback-time registry
      // check still owns the one-shot consume boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [cancellationId, cancellation] :
             objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              cancellation.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored ownership-acquisition cancellation has no live callback route.");
          }
          result.ownershipAcquisitionCancellationWorkItems.push_back({
              cancellation.requestingFederateId,
              objectInstanceHandle,
              cancellationId,
              cancellation.attributeHandles,
              callbackRoute->second,
          });
        }
      }
      // A Divestiture If Wanted transfer changes ownership synchronously, but
      // its Acquisition Notification remains callback-gated. Rebind each
      // durable notification to the current requester route after restore;
      // beginAttributeOwnershipDivestitureIfWantedNotification owns the
      // one-shot consume boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [notificationId, notification] :
             objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              notification.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored Divestiture If Wanted notification has no live callback route.");
          }
          result.ownershipDivestitureIfWantedWorkItems.push_back({
              notificationId,
              notification.receivingFederateId,
              objectInstanceHandle,
              notification.attributeHandles,
              notification.userSuppliedTag,
              callbackRoute->second,
          });
        }
      }
      // Confirm Divestiture transfers ownership synchronously, but its
      // Attribute Ownership Acquisition Notification remains callback-gated.
      // Rebind each durable notification to the current requester route only
      // after Federation Restored, preserving the confirming tag.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [notificationId, notification] :
             objectInstance.pendingConfirmDivestitureNotifications) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              notification.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored Confirm Divestiture notification has no live callback route.");
          }
          result.confirmDivestitureWorkItems.push_back({
              notificationId,
              notification.receivingFederateId,
              objectInstanceHandle,
              notification.attributeHandles,
              notification.userSuppliedTag,
              callbackRoute->second,
          });
        }
      }
      // An attribute transportation-type change is a separate requester
      // callback reservation. Rebind the durable request to the live route;
      // beginAttributeTransportationTypeChange still owns the one-shot
      // consume/commit boundary.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeTransportationTypeChanges) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored attribute transportation-type change has no live callback route.");
          }
          result.attributeTransportationTypeChangeWorkItems.push_back({
              request.requestingFederateId,
              objectInstanceHandle,
              requestId,
              callbackRoute->second,
          });
        }
      }
      // An interaction transportation-type change is a publisher callback
      // reservation independent of object-instance state. Rebind the saved
      // class identity to the current publisher route; the confirmation
      // callback owns the one-shot consume/commit boundary.
      for (auto const& [requestingFederateId, declarations] :
           federation->second.interactionDeclarations) {
        for (auto const& [interactionClassHandle, transportationName] :
             declarations.pendingInteractionTransportationTypeChanges) {
          static_cast<void>(transportationName);
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              requestingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored interaction transportation-type change has no live callback route.");
          }
          result.interactionTransportationTypeChangeWorkItems.push_back({
              requestingFederateId,
              interactionClassHandle,
              callbackRoute->second,
          });
        }
      }
      // Object-instance Request Attribute Value Update callbacks are the
      // remaining route-free application-request ledger in this slice. The
      // request record stays in the restored object until the callback route
      // reaches its begin boundary, where the registry consumes it exactly
      // once (including a stale/no-delivery outcome).
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedAttributeHandles,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateClassRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored object-class attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateClassProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedObjectClassHandle,
              request.requestedAttributeHandles,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
        for (auto const& [requestId, request] :
             objectInstance.pendingAttributeValueUpdateRegionalRequests) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              request.providingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored regional attribute value update request has no live provider callback route.");
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              request.providingFederateId);
          result.attributeValueUpdateRegionalProvideWorkItems.push_back({
              requestId,
              request.requestingFederateId,
              request.providingFederateId,
              objectInstanceHandle,
              request.requestedObjectClassHandle,
              request.requestedAttributeHandles,
              request.requestRegionsByAttribute,
              request.userSuppliedTag,
              callbackRoute->second,
              reportRoute == federation->second.serviceReportRoutes.end()
                  ? FederateServiceReportRoute{}
                  : reportRoute->second,
          });
        }
      }
      // Queued Request Attribute Ownership Assumption callbacks retain only
      // their route-free object/recipient/attribute/tag tuple. Rebind each
      // callback to the current recipient route after Federation Restored;
      // attributeOwnershipAssumptionDeliveryFor consumes the tuple exactly
      // once at callback entry and rechecks the live unowned state.
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        for (auto const& pending :
             objectInstance.pendingAttributeOwnershipAssumptionCallbacks) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              pending.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A restored ownership-assumption callback has no live recipient route.");
          }
          result.attributeOwnershipAssumptionWorkItems.push_back({
              pending.receivingFederateId,
              objectInstanceHandle,
              pending.attributeHandles,
              callbackRoute->second,
              pending.userSuppliedTag,
          });
        }
      }
      // Query Attribute Ownership result callbacks are a federation-scoped
      // application-request ledger. Rebind each saved result to the current
      // requester route only after Federation Restored, preserving the same
      // one-shot callback boundary as the other restored request families.
      for (auto const& [requestId, request] :
           federation->second.pendingAttributeOwnershipQueries) {
        auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
            request.requestingFederateId);
        if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
            !callbackRoute->second) {
          throw std::logic_error(
              "A restored Attribute Ownership query has no live callback route.");
        }
        AttributeOwnershipQueryRecipient work;
        work.requestId = requestId;
        work.receivingFederateId = request.requestingFederateId;
        work.objectInstanceHandle = request.objectInstanceHandle;
        work.reportKind = request.reportKind;
        work.owningFederateId = request.owningFederateId;
        work.attributeHandles = request.requestedAttributeHandles;
        work.callbackRoute = callbackRoute->second;
        result.attributeOwnershipQueryWorkItems.push_back(std::move(work));
      }
    }
    if (processLocalSnapshot) {
      // A process-local save keeps the complete private snapshot, but the
      // process endpoint still needs value-only ownership work so it can
      // route the callback to the current session rather than reviving a
      // saved callback closure.  Keep this family aligned with the
      // fresh-registry rebind above; the process service projects it after
      // Federation Restored through its existing ownership event queue.
      for (auto const& [objectInstanceHandle, objectInstance] :
           saved->second.objectInstances) {
        for (auto const& pending :
             objectInstance.pendingAttributeOwnershipAssumptionCallbacks) {
          auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
              pending.receivingFederateId);
          if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
              !callbackRoute->second) {
            throw std::logic_error(
                "A process-local restored ownership-assumption callback has no live recipient route.");
          }
          result.attributeOwnershipAssumptionWorkItems.push_back({
              pending.receivingFederateId,
              objectInstanceHandle,
              pending.attributeHandles,
              callbackRoute->second,
              pending.userSuppliedTag,
          });
        }
      }
    }
    // restoreFederationFromSnapshot clears the operation after the callback
    // work has been built, so build successful completion notifications first
    // and restore the state only after all preconditions have passed.
  } catch (...) {
    appendRestoreCompletionNotifications(
        federation->second,
        false,
        rti1516_2025::RTI_UNABLE_TO_RESTORE,
        result.notifications);
    federation->second.restoreOperation.reset();
    result.status = FederationRestoreControlStatus::restore_failed;
    return result;
  }

  // The state transition above clears restoreOperation, so emit the callback
  // records from the saved member set using the live routes now in the target.
  for (auto const& [federateId, member] : federation->second.members) {
    auto const route = federation->second.interactionCallbackRoutes.find(federateId);
    if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
      continue;
    }
    auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
    FederationRestoreNotification notification;
    notification.kind = FederationRestoreNotificationKind::completed;
    notification.receivingFederateId = federateId;
    notification.label = label;
    notification.federateName = member.name;
    notification.preRestoreFederateId = federateId;
    notification.postRestoreFederateId = federateId;
    notification.successful = true;
    notification.failureReason = rti1516_2025::RTI_UNABLE_TO_RESTORE;
    notification.callbackRoute = route->second;
    notification.publicServiceReportRoute =
        publicReportRoute == federation->second.publicServiceReportRoutes.end()
        ? FederatePublicServiceReportRoute{}
        : publicReportRoute->second;
    result.notifications.push_back(std::move(notification));
  }
  // Queue the reconstructed grant only after every successful restore
  // notification has been prepared. The adapter submits those callbacks first,
  // preserving a clear restored-before-grant boundary in both callback models.
  result.timeAdvanceGrantDispatches =
      scheduleEligibleTimeAdvanceGrants(federation->second);
  // Role-enable callbacks are likewise rebuilt only after the target time
  // coordinator has applied the saved image. The live factory can therefore
  // capture the target state's fresh callback epoch and stale pre-restore
  // role work cannot consume a restored generation.
  result.timeRoleEnableDispatches =
      scheduleRestoredTimeRoleEnableDispatches(federation->second);
  result.restoredQueuedReceiveOrderInteractions =
      federation->second.queuedReceiveOrderInteractions;
  result.restoredQueuedReceiveOrderAttributeUpdates =
      federation->second.queuedReceiveOrderAttributeUpdates;
  return result;
}
FederationRestoreControlResult EmbeddedFederationRegistry::federateRestoreNotComplete(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("federateRestoreNotComplete");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  auto const status = federation->second.restoreOperation->statuses.find(federateId);
  if (status == federation->second.restoreOperation->statuses.end() ||
      status->second != rti1516_2025::FEDERATE_RESTORING) {
    result.status = FederationRestoreControlStatus::restore_not_requested;
    return result;
  }
  appendRestoreCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::FEDERATE_REPORTED_FAILURE_DURING_RESTORE,
      result.notifications);
  federation->second.restoreOperation.reset();
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::abortFederationRestore(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("abortFederationRestore");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  if (!federation->second.restoreOperation.has_value()) {
    result.status = FederationRestoreControlStatus::restore_not_in_progress;
    return result;
  }
  appendRestoreCompletionNotifications(
      federation->second,
      false,
      rti1516_2025::RESTORE_ABORTED,
      result.notifications);
  federation->second.restoreOperation.reset();
  return result;
}

FederationRestoreControlResult EmbeddedFederationRegistry::queryFederationRestoreStatus(
    std::wstring const& federationName,
    std::uint64_t federateId) {
  auto instrumentationScope = beginInstrumentation("queryFederationRestoreStatus");
  FederationRestoreControlResult result;

  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = FederationRestoreControlStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = FederationRestoreControlStatus::federate_not_member;
    return result;
  }
  if (federation->second.saveOperation.has_value()) {
    result.status = FederationRestoreControlStatus::save_in_progress;
    return result;
  }
  auto const route = federation->second.interactionCallbackRoutes.find(federateId);
  if (route == federation->second.interactionCallbackRoutes.end() || !route->second) {
    result.status = FederationRestoreControlStatus::callback_route_missing;
    return result;
  }

  FederationRestoreNotification notification;
  notification.kind = FederationRestoreNotificationKind::status;
  notification.receivingFederateId = federateId;
  notification.callbackRoute = route->second;
  auto const reportRoute = federation->second.serviceReportRoutes.find(federateId);
  notification.serviceReportRoute =
      reportRoute == federation->second.serviceReportRoutes.end()
      ? FederateServiceReportRoute{}
      : reportRoute->second;
  auto const publicReportRoute = federation->second.publicServiceReportRoutes.find(federateId);
  notification.publicServiceReportRoute =
      publicReportRoute == federation->second.publicServiceReportRoutes.end()
      ? FederatePublicServiceReportRoute{}
      : publicReportRoute->second;
  if (federation->second.restoreOperation.has_value()) {
    for (auto const& [participantId, status] : federation->second.restoreOperation->statuses) {
      notification.statuses.push_back({participantId, participantId, status});
    }
  } else {
    for (auto const& [memberId, member] : federation->second.members) {
      static_cast<void>(member);
      notification.statuses.push_back({
          memberId,
          0,
          rti1516_2025::NO_RESTORE_IN_PROGRESS,
      });
    }
  }
  result.notifications.push_back(std::move(notification));
  return result;
}

}  // namespace umbra::detail
