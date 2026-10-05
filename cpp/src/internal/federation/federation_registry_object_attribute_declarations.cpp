#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>

namespace umbra::detail {
ObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributePublication(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    bool published) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }
  if (!validObjectClassAttributes(federation->second, objectClassHandle, attributeHandles)) {
    return {ObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (!published) {
    if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
      return ObjectClassAttributeDeclarationStatus::applied;
    }
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass == declarations->second.byObjectClass.end()) {
      return ObjectClassAttributeDeclarationStatus::applied;
    }

    auto const currentlyPublished = publishedObjectClassAttributes(
        federation->second,
        federateId,
        objectClassHandle);
    if (!currentlyPublished) {
      return ObjectClassAttributeDeclarationStatus::inconsistent_catalog;
    }

    // IEEE 1516.1-2025 5.3.3(f) forbids removal of a publication needed by a
    // still-pending acquisition.  In this bounded profile both acquisition
    // forms are recorded against the requesting federate's known class, so
    // only an unpublication of that exact known class can remove their
    // publication precondition.  Check the whole call before changing any
    // declaration state, including implicit HLAprivilegeToDeleteObject.
    for (std::uint64_t const attributeHandle : attributeHandles) {
      if (!currentlyPublished->contains(attributeHandle)) {
        continue;
      }
      for (auto const& [objectInstanceHandle, objectInstance] :
           federation->second.objectInstances) {
        static_cast<void>(objectInstanceHandle);
        if (objectInstance.deleteAccepted) {
          continue;
        }
        auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(federateId);
        if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end() ||
            knownClass->second != objectClassHandle) {
          continue;
        }
        for (auto const& [notificationId, notification] :
             objectInstance.pendingConfirmDivestitureNotifications) {
          static_cast<void>(notificationId);
          if (notification.receivingFederateId == federateId &&
              notification.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        // Divestiture If Wanted moves ownership synchronously, but the new
        // owner still has an Acquisition Notification outstanding. Its
        // reservation must therefore be checked before the normal owner fast
        // path below, which otherwise applies only after a callback boundary.
        for (auto const& [notificationId, notification] :
             objectInstance.pendingAttributeOwnershipDivestitureIfWantedNotifications) {
          static_cast<void>(notificationId);
          if (notification.receivingFederateId == federateId &&
              notification.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
        if (owner != objectInstance.attributeOwnersByHandle.end() && owner->second == federateId) {
          continue;
        }
        for (auto const& [requestId, pending] :
             objectInstance.pendingAttributeOwnershipAcquisitionIfAvailableRequests) {
          static_cast<void>(requestId);
          if (pending.requestingFederateId == federateId &&
              pending.desiredAttributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        for (auto const& [requestId, pending] :
             objectInstance.pendingAttributeOwnershipAcquisitionRequests) {
          static_cast<void>(requestId);
          if (pending.requestingFederateId == federateId &&
              pending.desiredAttributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
        for (auto const& [cancellationId, cancellation] :
             objectInstance.pendingAttributeOwnershipAcquisitionCancellations) {
          static_cast<void>(cancellationId);
          if (cancellation.requestingFederateId == federateId &&
              cancellation.attributeHandles.contains(attributeHandle)) {
            return ObjectClassAttributeDeclarationStatus::ownership_acquisition_pending;
          }
        }
      }
    }

    auto revised = perClass->second.explicitlyPublishedAttributes;
    bool privilegeToDeleteExplicitlyUnpublished =
        perClass->second.privilegeToDeleteExplicitlyUnpublished;
    auto const objectClassName = federation->second.objectClassHandles->nameFor(objectClassHandle);
    if (!objectClassName) {
      return ObjectClassAttributeDeclarationStatus::object_class_not_defined;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      revised.erase(attributeHandle);
      auto const attributeName = federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *objectClassName,
          attributeHandle);
      if (!attributeName) {
        return ObjectClassAttributeDeclarationStatus::attribute_not_defined;
      }
      if (*attributeName == "HLAprivilegeToDeleteObject") {
        privilegeToDeleteExplicitlyUnpublished = true;
      }
    }
    perClass->second.explicitlyPublishedAttributes.swap(revised);
    perClass->second.privilegeToDeleteExplicitlyUnpublished =
        privilegeToDeleteExplicitlyUnpublished;

    // IEEE 1516.1-2025 5.3.3 requires an unpublishing federate to lose
    // ownership of every corresponding instance attribute.  Keep that
    // transition federation-wide, including instances registered at a
    // subclass, so a later update cannot use a stale ownership record after
    // the publication boundary has been removed.
    for (auto& [objectInstanceHandle, objectInstance] :
         federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      if (!objectInstanceRegisteredAtOrBelowClass(
              federation->second,
              objectInstance.registeredObjectClassHandle,
              objectClassHandle)) {
        continue;
      }
      for (std::uint64_t const attributeHandle : attributeHandles) {
        auto assumptionSearch =
            objectInstance.ownershipAssumptionRecipientsByAttribute.find(attributeHandle);
        if (assumptionSearch !=
            objectInstance.ownershipAssumptionRecipientsByAttribute.end()) {
          assumptionSearch->second.erase(federateId);
        }
        auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
        if (owner != objectInstance.attributeOwnersByHandle.end() &&
            owner->second == federateId) {
          objectInstance.attributeOwnersByHandle.erase(owner);
          clearUpdateRegionAssociation(objectInstance, attributeHandle);
          objectInstance.attributeTransportationTypes.erase(attributeHandle);
          objectInstance.attributeOrderTypes.erase(attributeHandle);
          for (auto pending = objectInstance.pendingAttributeTransportationTypeChanges.begin();
               pending != objectInstance.pendingAttributeTransportationTypeChanges.end();) {
            pending->second.attributeHandles.erase(attributeHandle);
            if (pending->second.attributeHandles.empty()) {
              pending = objectInstance.pendingAttributeTransportationTypeChanges.erase(pending);
            } else {
              ++pending;
            }
          }
        }
      }
    }

    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
      declarations->second.byObjectClass.erase(perClass);
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
    // Unpublishing an owned regional attribute clears its update-region
    // association above.  Recompute the region usage ledger before returning
    // so a region whose last association was removed is immediately
    // deletable, while subscriptions and unrelated associations remain
    // protected by the in-use guard.
    refreshRegionUsage(federation->second);
    return ObjectClassAttributeDeclarationStatus::applied;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.explicitlyPublishedAttributes;
    bool const wasUnpublished = revised.empty();
    for (std::uint64_t const attributeHandle : attributeHandles) {
      static_cast<void>(revised.insert(attributeHandle));
    }
    bool privilegeToDeleteExplicitlyUnpublished =
        perClass->second.privilegeToDeleteExplicitlyUnpublished;
    if (wasUnpublished && !revised.empty()) {
      // A new publication establishment starts a fresh implicit-privilege
      // epoch. The limited registration slice snapshots this state.
      privilegeToDeleteExplicitlyUnpublished = false;
    }
    auto const objectClassName = federation->second.objectClassHandles->nameFor(objectClassHandle);
    if (!objectClassName) {
      return ObjectClassAttributeDeclarationStatus::object_class_not_defined;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      auto const attributeName = federation->second.attributeHandles->nameFor(
          federation->second.definition.catalog.get(),
          *objectClassName,
          attributeHandle);
      if (!attributeName) {
        return ObjectClassAttributeDeclarationStatus::attribute_not_defined;
      }
      if (*attributeName == "HLAprivilegeToDeleteObject") {
        privilegeToDeleteExplicitlyUnpublished = false;
      }
    }
    perClass->second.explicitlyPublishedAttributes.swap(revised);
    perClass->second.privilegeToDeleteExplicitlyUnpublished =
        privilegeToDeleteExplicitlyUnpublished;
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  return ObjectClassAttributeDeclarationStatus::applied;
}

ObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::setObjectClassAttributeSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::optional<bool> active,
    std::string const& updateRateDesignator) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }
  if (!validObjectClassAttributes(federation->second, objectClassHandle, attributeHandles)) {
    return {ObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  std::optional<std::string> normalizedDesignator;
  if (active) {
    if (!federation->second.definition.catalog) {
      return {ObjectClassAttributeDeclarationStatus::inconsistent_catalog};
    }
    normalizedDesignator = normalizedUpdateRateDesignator(
        *federation->second.definition.catalog,
        updateRateDesignator);
    if (!normalizedDesignator) {
      return {ObjectClassAttributeDeclarationStatus::invalid_update_rate_designator};
    }
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (!active) {
    if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
      return {};
    }
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass == declarations->second.byObjectClass.end()) {
      return {};
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      perClass->second.subscribedAttributes.erase(attributeHandle);
      perClass->second.subscribedUpdateRateDesignators.erase(attributeHandle);
    }
    declarations->second.subscriptionGeneration =
        federation->second.nextSubscriptionGeneration++;
    if (perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
      declarations->second.byObjectClass.erase(perClass);
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
    auto result = ObjectClassAttributeSubscriptionScopePlan{};
    result.recipients = objectInstanceScopeChangesForSubscription(
        federation->second,
        federateId,
        attributeHandles,
        previousDeclarations);
    result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
        federation->second,
        result.recipients);
    auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
        federation->second,
        federateId,
        attributeHandles,
        nullptr,
        nullptr,
        nullptr,
        &previousDeclarations);
    result.attributeRelevanceAdvisories.insert(
        result.attributeRelevanceAdvisories.end(),
        transitionAdvisories.begin(),
        transitionAdvisories.end());
    return result;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.subscribedAttributes;
    auto revisedDesignators = perClass->second.subscribedUpdateRateDesignators;
    auto const storedDesignator = storedUpdateRateDesignator(
        updateRateDesignator,
        *normalizedDesignator);
    for (std::uint64_t const attributeHandle : attributeHandles) {
      revised.insert_or_assign(attributeHandle, *active);
      revisedDesignators.insert_or_assign(attributeHandle, storedDesignator);
    }
    perClass->second.subscribedAttributes.swap(revised);
    perClass->second.subscribedUpdateRateDesignators.swap(revisedDesignators);
    state->second.subscriptionGeneration = federation->second.nextSubscriptionGeneration++;
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  auto result = ObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      federateId,
      attributeHandles,
      nullptr,
      nullptr,
      nullptr,
      &previousDeclarations);
  result.attributeRelevanceAdvisories.insert(
      result.attributeRelevanceAdvisories.end(),
      transitionAdvisories.begin(),
      transitionAdvisories.end());
  return result;
}

ObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::set<std::uint64_t> const& attributeHandles,
    std::optional<bool> active,
    std::string const& updateRateDesignator) {
  return setObjectClassAttributeSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributeHandles,
             active,
             updateRateDesignator)
      .status;
}

RegionalObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active,
    std::string const& updateRateDesignator) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionalObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionalObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {RegionalObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }

  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return {RegionalObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  if (attributesAndRegions.empty()) {
    return {};
  }
  if (!federation->second.definition.catalog) {
    return {RegionalObjectClassAttributeDeclarationStatus::inconsistent_catalog};
  }
  auto const normalizedDesignator = normalizedUpdateRateDesignator(
      *federation->second.definition.catalog,
      updateRateDesignator);
  if (!normalizedDesignator) {
    return {RegionalObjectClassAttributeDeclarationStatus::invalid_update_rate_designator};
  }

  auto const availableDimensions = availableObjectClassDimensions(
      federation->second,
      objectClassHandle);
  if (!availableDimensions) {
    return {RegionalObjectClassAttributeDeclarationStatus::inconsistent_catalog};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {RegionalObjectClassAttributeDeclarationStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region_context};
      }
    }
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto [state, insertedState] =
      federation->second.objectClassAttributeDeclarations.try_emplace(federateId);
  auto [perClass, insertedClass] = state->second.byObjectClass.try_emplace(objectClassHandle);
  try {
    auto revised = perClass->second.regionalSubscribedAttributes;
    auto revisedDesignators = perClass->second.regionalSubscribedUpdateRateDesignators;
    auto const storedDesignator = storedUpdateRateDesignator(
        updateRateDesignator,
        *normalizedDesignator);
    for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
      auto& subscriptions = revised[attributeHandle];
      auto& designators = revisedDesignators[attributeHandle];
      for (std::uint64_t const regionHandle : regionHandles) {
        subscriptions.insert_or_assign(regionHandle, active);
        designators.insert_or_assign(regionHandle, storedDesignator);
      }
      if (subscriptions.empty()) {
        revised.erase(attributeHandle);
        revisedDesignators.erase(attributeHandle);
      }
    }
    perClass->second.regionalSubscribedAttributes.swap(revised);
    perClass->second.regionalSubscribedUpdateRateDesignators.swap(revisedDesignators);
    state->second.subscriptionGeneration = federation->second.nextSubscriptionGeneration++;
    refreshRegionUsage(federation->second);
  } catch (...) {
    if (insertedClass && perClass->second.explicitlyPublishedAttributes.empty() &&
        perClass->second.subscribedAttributes.empty() &&
        perClass->second.subscribedUpdateRateDesignators.empty() &&
        perClass->second.regionalSubscribedAttributes.empty() &&
        perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
      state->second.byObjectClass.erase(perClass);
    }
    if (insertedState && state->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(state);
    }
    throw;
  }
  auto result = RegionalObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      federateId,
      attributeHandles,
      nullptr,
      nullptr,
      nullptr,
      &previousDeclarations);
  result.attributeRelevanceAdvisories.insert(
      result.attributeRelevanceAdvisories.end(),
      transitionAdvisories.begin(),
      transitionAdvisories.end());
  return result;
}

RegionalObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::setObjectClassAttributeRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions,
    bool active,
    std::string const& updateRateDesignator) {
  return setObjectClassAttributeRegionalSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributesAndRegions,
             active,
             updateRateDesignator)
      .status;
}

RegionalObjectClassAttributeSubscriptionScopePlan
EmbeddedFederationRegistry::removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionalObjectClassAttributeDeclarationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionalObjectClassAttributeDeclarationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {RegionalObjectClassAttributeDeclarationStatus::object_class_not_defined};
  }

  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          objectClassHandle,
          attributeHandles)) {
    return {RegionalObjectClassAttributeDeclarationStatus::attribute_not_defined};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {RegionalObjectClassAttributeDeclarationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {RegionalObjectClassAttributeDeclarationStatus::region_not_created_by_this_federate};
      }
    }
  }

  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  auto const previous = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (previous != federation->second.objectClassAttributeDeclarations.end()) {
    previousDeclarations = previous->second;
  }

  auto declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (declarations != federation->second.objectClassAttributeDeclarations.end()) {
    auto perClass = declarations->second.byObjectClass.find(objectClassHandle);
    if (perClass != declarations->second.byObjectClass.end()) {
      bool subscriptionChanged = false;
      for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
        auto regional = perClass->second.regionalSubscribedAttributes.find(attributeHandle);
        if (regional == perClass->second.regionalSubscribedAttributes.end()) {
          continue;
        }
        for (std::uint64_t const regionHandle : regionHandles) {
          if (regional->second.erase(regionHandle) == 0U) {
            continue;
          }
          subscriptionChanged = true;
          auto regionalDesignators =
              perClass->second.regionalSubscribedUpdateRateDesignators.find(attributeHandle);
          if (regionalDesignators !=
              perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            regionalDesignators->second.erase(regionHandle);
            if (regionalDesignators->second.empty()) {
              perClass->second.regionalSubscribedUpdateRateDesignators.erase(
                  regionalDesignators);
            }
          }
        }
        if (regional->second.empty()) {
          perClass->second.regionalSubscribedAttributes.erase(regional);
        }
      }
      // A regional declaration mutation starts a new receiver lifetime for
      // update-rate admission.  Without a fresh generation, an immediate
      // unsubscribe/re-subscribe could inherit the old wall-clock gate and
      // suppress the first update in the new declaration epoch.
      if (subscriptionChanged) {
        declarations->second.subscriptionGeneration =
            federation->second.nextSubscriptionGeneration++;
      }
      if (perClass->second.explicitlyPublishedAttributes.empty() &&
          perClass->second.subscribedAttributes.empty() &&
          perClass->second.subscribedUpdateRateDesignators.empty() &&
          perClass->second.regionalSubscribedAttributes.empty() &&
          perClass->second.regionalSubscribedUpdateRateDesignators.empty()) {
        declarations->second.byObjectClass.erase(perClass);
      }
    }
    if (declarations->second.byObjectClass.empty()) {
      federation->second.objectClassAttributeDeclarations.erase(declarations);
    }
  }
  refreshRegionUsage(federation->second);
  auto result = RegionalObjectClassAttributeSubscriptionScopePlan{};
  result.recipients = objectInstanceScopeChangesForSubscription(
      federation->second,
      federateId,
      attributeHandles,
      previousDeclarations);
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      federateId,
      attributeHandles,
      nullptr,
      nullptr,
      nullptr,
      &previousDeclarations);
  result.attributeRelevanceAdvisories.insert(
      result.attributeRelevanceAdvisories.end(),
      transitionAdvisories.begin(),
      transitionAdvisories.end());
  return result;
}

RegionalObjectClassAttributeDeclarationStatus
EmbeddedFederationRegistry::removeObjectClassAttributeRegionalSubscription(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  return removeObjectClassAttributeRegionalSubscriptionWithScopeChanges(
             federationName,
             federateId,
             objectClassHandle,
             attributesAndRegions)
      .status;
}

std::optional<ObjectClassAttributeDeclarationSnapshot>
EmbeddedFederationRegistry::objectClassAttributeDeclarationFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId) ||
      !validObjectClass(federation->second, objectClassHandle)) {
    return std::nullopt;
  }
  auto const declarations = federation->second.objectClassAttributeDeclarations.find(federateId);
  if (declarations == federation->second.objectClassAttributeDeclarations.end()) {
    return ObjectClassAttributeDeclarationSnapshot{};
  }
  auto const perClass = declarations->second.byObjectClass.find(objectClassHandle);
  if (perClass == declarations->second.byObjectClass.end()) {
    return ObjectClassAttributeDeclarationSnapshot{};
  }
  return ObjectClassAttributeDeclarationSnapshot{
      perClass->second.explicitlyPublishedAttributes,
      perClass->second.subscribedAttributes,
      perClass->second.subscribedUpdateRateDesignators,
      declarations->second.subscriptionGeneration,
  };
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::publishedObjectClassAttributeHandles(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId) ||
      !validObjectClass(federation->second, objectClassHandle)) {
    return std::nullopt;
  }
  return publishedObjectClassAttributes(
      federation->second,
      federateId,
      objectClassHandle);
}

}  // namespace umbra::detail
