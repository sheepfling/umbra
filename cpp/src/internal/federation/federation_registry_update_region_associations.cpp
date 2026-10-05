#include "internal/federation/federation_registry.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>

namespace umbra::detail {
ObjectInstanceRegionAssociationScopePlan
EmbeddedFederationRegistry::associateRegionsForUpdatesWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegionAssociationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::object_instance_not_known};
  }
  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          instance->second.registeredObjectClassHandle,
          attributeHandles)) {
    return {ObjectInstanceRegionAssociationStatus::attribute_not_defined};
  }
  if (attributesAndRegions.empty()) {
    return {};
  }
  auto const availableDimensions = availableObjectClassDimensions(
      federation->second,
      instance->second.registeredObjectClassHandle);
  if (!availableDimensions) {
    return {ObjectInstanceRegionAssociationStatus::inconsistent_catalog};
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {ObjectInstanceRegionAssociationStatus::region_not_created_by_this_federate};
      }
      if (!region->second.specificationCommitted ||
          region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (!std::includes(
              availableDimensions->begin(),
              availableDimensions->end(),
              region->second.dimensionHandles.begin(),
              region->second.dimensionHandles.end())) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region_context};
      }
    }
  }
  auto revisedAssociations = instance->second.updateRegionsByAttribute;
  auto revisedDeferredAssociations = instance->second.deferredUpdateRegionsByFederate;
  std::set<std::uint64_t> activeAttributeHandles;
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == federateId) {
      activeAttributeHandles.insert(attributeHandle);
      auto& associated = revisedAssociations[attributeHandle];
      associated.insert(regionHandles.begin(), regionHandles.end());
      if (associated.empty()) {
        revisedAssociations.erase(attributeHandle);
      }
      auto deferredByAttribute = revisedDeferredAssociations.find(federateId);
      if (deferredByAttribute != revisedDeferredAssociations.end()) {
        deferredByAttribute->second.erase(attributeHandle);
        if (deferredByAttribute->second.empty()) {
          revisedDeferredAssociations.erase(deferredByAttribute);
        }
      }
    } else {
      auto& deferred = revisedDeferredAssociations[federateId][attributeHandle];
      deferred.insert(regionHandles.begin(), regionHandles.end());
      if (deferred.empty()) {
        auto deferredByFederate = revisedDeferredAssociations.find(federateId);
        if (deferredByFederate != revisedDeferredAssociations.end()) {
          deferredByFederate->second.erase(attributeHandle);
          if (deferredByFederate->second.empty()) {
            revisedDeferredAssociations.erase(deferredByFederate);
          }
        }
      }
    }
  }
  // A deferred association is an ownership ledger mutation only. It must not
  // create discovery, scope, or relevance-advisory transitions while another
  // federate still owns the attribute.
  if (activeAttributeHandles.empty()) {
    instance->second.deferredUpdateRegionsByFederate.swap(revisedDeferredAssociations);
    refreshRegionUsage(federation->second);
    return {};
  }
  auto result = ObjectInstanceRegionAssociationScopePlan{};
  // Association is also a discovery boundary.  An object registered with the
  // private default source region can become eligible for an existing
  // explicit regional subscription when its owner adds an overlapping update
  // region.  Compare the old and revised association maps before swapping the
  // committed state, and reserve each discovery in this same transaction.
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (receivingFederateId == instance->second.producingFederateId ||
          instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          instance->second.pendingDiscoveryFederates.contains(receivingFederateId)) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      auto const wasDiscoverable = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId,
          nullptr,
          &instance->second.updateRegionsByAttribute);
      if (wasDiscoverable) {
        continue;
      }
      auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId,
          nullptr,
          &revisedAssociations);
      if (!discoveredClass) {
        continue;
      }
      auto const [pending, insertedPending] =
          instance->second.pendingDiscoveryFederates.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      auto const reportRoute = federation->second.serviceReportRoutes.find(
          receivingFederateId);
      try {
        result.discoveries.push_back({
            receivingFederateId,
            instance->second.handle,
            *discoveredClass,
            instance->second.name,
            instance->second.producingFederateId,
            callbackRoute->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
        });
      } catch (...) {
        instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& discovery : result.discoveries) {
      auto const objectInstance = federation->second.objectInstances.find(
          discovery.objectInstanceHandle);
      if (objectInstance != federation->second.objectInstances.end()) {
        objectInstance->second.pendingDiscoveryFederates.erase(
            discovery.receivingFederateId);
      }
    }
    throw;
  }
  result.recipients = objectInstanceScopeChangesForAssociation(
      federation->second,
      instance->second,
      activeAttributeHandles,
      revisedAssociations);
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      0,
      activeAttributeHandles,
      nullptr,
      &instance->second.updateRegionsByAttribute,
      &revisedAssociations,
      nullptr,
      objectInstanceHandle);
  result.attributeRelevanceAdvisories.insert(
      result.attributeRelevanceAdvisories.end(),
      transitionAdvisories.begin(),
      transitionAdvisories.end());
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  instance->second.deferredUpdateRegionsByFederate.swap(revisedDeferredAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::associateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  auto instrumentationScope = beginInstrumentation("associateRegionsForUpdates");
  return associateRegionsForUpdatesWithScopeChanges(
             federationName,
             federateId,
             objectInstanceHandle,
             attributesAndRegions)
      .status;
}

ObjectInstanceRegionAssociationScopePlan
EmbeddedFederationRegistry::unassociateRegionsForUpdatesWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegionAssociationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::federate_not_member};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(federateId)) {
    return {ObjectInstanceRegionAssociationStatus::object_instance_not_known};
  }
  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, regions] : attributesAndRegions) {
    static_cast<void>(regions);
    attributeHandles.insert(attributeHandle);
  }
  if (!validObjectClassAttributes(
          federation->second,
          instance->second.registeredObjectClassHandle,
          attributeHandles)) {
    return {ObjectInstanceRegionAssociationStatus::attribute_not_defined};
  }
  auto revisedAssociations = instance->second.updateRegionsByAttribute;
  auto revisedDeferredAssociations = instance->second.deferredUpdateRegionsByFederate;
  std::set<std::uint64_t> activeAttributeHandles;
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    static_cast<void>(attributeHandle);
    for (std::uint64_t const regionHandle : regionHandles) {
      auto const region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        return {ObjectInstanceRegionAssociationStatus::invalid_region};
      }
      if (region->second.ownerFederateId != federateId) {
        return {ObjectInstanceRegionAssociationStatus::region_not_created_by_this_federate};
      }
    }
  }
  for (auto const& [attributeHandle, regionHandles] : attributesAndRegions) {
    auto const owner = instance->second.attributeOwnersByHandle.find(attributeHandle);
    if (owner != instance->second.attributeOwnersByHandle.end() &&
        owner->second == federateId) {
      activeAttributeHandles.insert(attributeHandle);
      auto associated = revisedAssociations.find(attributeHandle);
      if (associated == revisedAssociations.end()) {
        continue;
      }
      for (std::uint64_t const regionHandle : regionHandles) {
        associated->second.erase(regionHandle);
      }
      if (associated->second.empty()) {
        revisedAssociations.erase(associated);
      }
    } else {
      auto deferredByFederate = revisedDeferredAssociations.find(federateId);
      if (deferredByFederate == revisedDeferredAssociations.end()) {
        continue;
      }
      auto deferred = deferredByFederate->second.find(attributeHandle);
      if (deferred == deferredByFederate->second.end()) {
        continue;
      }
      for (std::uint64_t const regionHandle : regionHandles) {
        deferred->second.erase(regionHandle);
      }
      if (deferred->second.empty()) {
        deferredByFederate->second.erase(deferred);
      }
      if (deferredByFederate->second.empty()) {
        revisedDeferredAssociations.erase(deferredByFederate);
      }
    }
  }
  // Removing a deferred association is a private ownership ledger mutation;
  // only changes to the current owner's active map can alter scope or
  // discovery.
  if (activeAttributeHandles.empty()) {
    instance->second.deferredUpdateRegionsByFederate.swap(revisedDeferredAssociations);
    refreshRegionUsage(federation->second);
    return {};
  }
  auto result = ObjectInstanceRegionAssociationScopePlan{};
  // Removing an update-region association can expose the private default
  // source realization to an existing regional subscriber.  Compare the old
  // and revised maps before swapping the object state so an object that was
  // previously disjoint receives the same one-time discovery reservation as
  // the additive association path.
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (receivingFederateId == instance->second.producingFederateId ||
          instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          instance->second.pendingDiscoveryFederates.contains(receivingFederateId)) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      auto const wasDiscoverable = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId,
          nullptr,
          &instance->second.updateRegionsByAttribute);
      if (wasDiscoverable) {
        continue;
      }
      auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId,
          nullptr,
          &revisedAssociations);
      if (!discoveredClass) {
        continue;
      }
      auto const [pending, insertedPending] =
          instance->second.pendingDiscoveryFederates.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      auto const reportRoute = federation->second.serviceReportRoutes.find(
          receivingFederateId);
      try {
        result.discoveries.push_back({
            receivingFederateId,
            instance->second.handle,
            *discoveredClass,
            instance->second.name,
            instance->second.producingFederateId,
            callbackRoute->second,
            reportRoute == federation->second.serviceReportRoutes.end()
                ? FederateServiceReportRoute{}
                : reportRoute->second,
        });
      } catch (...) {
        instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& discovery : result.discoveries) {
      auto const objectInstance = federation->second.objectInstances.find(
          discovery.objectInstanceHandle);
      if (objectInstance != federation->second.objectInstances.end()) {
        objectInstance->second.pendingDiscoveryFederates.erase(
            discovery.receivingFederateId);
      }
    }
    throw;
  }
  result.recipients = objectInstanceScopeChangesForAssociation(
      federation->second,
      instance->second,
      activeAttributeHandles,
      revisedAssociations);
  result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
      federation->second,
      result.recipients);
  auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      0,
      activeAttributeHandles,
      nullptr,
      &instance->second.updateRegionsByAttribute,
      &revisedAssociations,
      nullptr,
      objectInstanceHandle);
  result.attributeRelevanceAdvisories.insert(
      result.attributeRelevanceAdvisories.end(),
      transitionAdvisories.begin(),
      transitionAdvisories.end());
  instance->second.updateRegionsByAttribute.swap(revisedAssociations);
  instance->second.deferredUpdateRegionsByFederate.swap(revisedDeferredAssociations);
  refreshRegionUsage(federation->second);
  return result;
}

ObjectInstanceRegionAssociationStatus EmbeddedFederationRegistry::unassociateRegionsForUpdates(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& attributesAndRegions) {
  auto instrumentationScope = beginInstrumentation("unassociateRegionsForUpdates");
  return unassociateRegionsForUpdatesWithScopeChanges(
             federationName,
             federateId,
             objectInstanceHandle,
             attributesAndRegions)
      .status;
}

}  // namespace umbra::detail
