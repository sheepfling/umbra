#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_constants.hpp"
#include "internal/federation/federation_state_image.hpp"

#include "internal/fom/fom_catalog.hpp"
#include "internal/fom/fdd_document.hpp"
#include "internal/fom/hla_names.hpp"
#include "internal/time/federation_time_bounds.hpp"
#include "internal/time/federation_time_grant_policy.hpp"
#include "internal/handles/handle_variable_array_encoding.hpp"
#include "internal/runtime/utf8_string.hpp"

#include <RTI/encoding/BasicDataElements.h>
#include <RTI/time/HLAlogicalTimeFactoryFactory.h>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>

namespace umbra::detail {

RegionCreateResult EmbeddedFederationRegistry::createRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& dimensionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return {RegionServiceStatus::inconsistent_catalog};
  }

  // IEEE 1516.1-2025 §9.2 specifies a set of dimension designators and does
  // not impose a non-empty cardinality rule.  An empty set therefore remains
  // a valid zero-dimensional region template; InvalidDimensionHandle applies
  // to a supplied designator that cannot be resolved in this federation.  Do
  // not import the sibling Python backend's stricter InvalidRegionContext
  // branch into the official C++ surface.
  for (std::uint64_t const dimensionHandle : dimensionHandles) {
    auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
    if (!dimensionName ||
        federation->second.definition.catalog->dimension(*dimensionName) == nullptr) {
      return {RegionServiceStatus::invalid_dimension};
    }
  }

  if (federation->second.nextRegionHandle == 0) {
    return {RegionServiceStatus::invalid_region};
  }
  std::uint64_t const regionHandle = federation->second.nextRegionHandle++;
  federation->second.regions.emplace(
      regionHandle,
      Federation::Region{federateId, dimensionHandles, {}, {}, false, false});
  return {RegionServiceStatus::applied, regionHandle};
}

RegionScopeChangePlan EmbeddedFederationRegistry::commitRegionModificationsWithScopeChanges(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& regionHandles) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return {RegionServiceStatus::inconsistent_catalog};
  }

  for (std::uint64_t const regionHandle : regionHandles) {
    auto const region = federation->second.regions.find(regionHandle);
    if (region == federation->second.regions.end()) {
      return {RegionServiceStatus::invalid_region};
    }
    if (region->second.ownerFederateId != federateId) {
      return {RegionServiceStatus::region_not_created_by_this_federate};
    }
    if (region->second.pendingRangeBounds.size() != region->second.dimensionHandles.size()) {
      return {RegionServiceStatus::incomplete_region};
    }
    for (std::uint64_t const dimensionHandle : region->second.dimensionHandles) {
      auto const pending = region->second.pendingRangeBounds.find(dimensionHandle);
      auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
      auto const* dimension = dimensionName
          ? federation->second.definition.catalog->dimension(*dimensionName)
          : nullptr;
      if (pending == region->second.pendingRangeBounds.end() || dimension == nullptr ||
          pending->second.lowerBound >= pending->second.upperBound ||
          pending->second.upperBound > dimension->upperBound) {
        return {RegionServiceStatus::invalid_range_bound};
      }
    }
  }

  std::map<std::uint64_t, RegionSpecificationSnapshot> previousRegions;
  for (std::uint64_t const regionHandle : regionHandles) {
    auto const& region = federation->second.regions.at(regionHandle);
    previousRegions.emplace(
        regionHandle,
        RegionSpecificationSnapshot{
            region.dimensionHandles,
            region.committedRangeBounds,
            region.specificationCommitted,
        });
  }

  RegionScopeChangePlan result;
  result.status = RegionServiceStatus::applied;
  // The committed region specification and the pending discovery/scope
  // ledgers form one service transaction.  Planning can allocate while it
  // walks the object and declaration state; if that planning throws, expose
  // the pre-service region state rather than leaving a partially committed
  // subset behind.  Swapping the saved maps back is non-allocating, so the
  // rollback remains usable while unwinding an allocation failure.
  auto rollback = [&] {
    for (auto& [regionHandle, previous] : previousRegions) {
      auto region = federation->second.regions.find(regionHandle);
      if (region == federation->second.regions.end()) {
        continue;
      }
      region->second.committedRangeBounds.swap(previous.committedRangeBounds);
      region->second.specificationCommitted = previous.specificationCommitted;
    }
    for (auto const& discovery : result.discoveries) {
      auto const objectInstance = federation->second.objectInstances.find(
          discovery.objectInstanceHandle);
      if (objectInstance != federation->second.objectInstances.end()) {
        objectInstance->second.pendingDiscoveryFederates.erase(
            discovery.receivingFederateId);
      }
    }
  };

  try {
    for (std::uint64_t const regionHandle : regionHandles) {
      auto& region = federation->second.regions.at(regionHandle);
      region.committedRangeBounds = region.pendingRangeBounds;
      region.specificationCommitted = true;
    }

    // Region mutation is also a discovery boundary.  A receiver can have a
    // retained regional subscription while an already-registered object is
    // outside its committed source range; moving that subscription into an
    // overlapping range must plan Discover Object Instance now.  Evaluate the
    // prior and current candidates against the same federation state, overriding
    // only the regions in this commit.
    try {
      for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
        static_cast<void>(objectInstanceHandle);
        if (objectInstance.deleteAccepted) {
          continue;
        }
        for (auto const& [receivingFederateId, membership] : federation->second.members) {
          static_cast<void>(membership);
          if (receivingFederateId == objectInstance.producingFederateId ||
              objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
              objectInstance.pendingDiscoveryFederates.contains(receivingFederateId)) {
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
              objectInstance,
              receivingFederateId,
              &previousRegions);
          if (wasDiscoverable) {
            continue;
          }
          auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
              federation->second,
              objectInstance,
              receivingFederateId);
          if (!discoveredClass) {
            continue;
          }
          auto const [pending, insertedPending] =
              objectInstance.pendingDiscoveryFederates.insert(receivingFederateId);
          static_cast<void>(pending);
          if (!insertedPending) {
            continue;
          }
          auto const reportRoute = federation->second.serviceReportRoutes.find(
              receivingFederateId);
          try {
            result.discoveries.push_back({
                receivingFederateId,
                objectInstance.handle,
                *discoveredClass,
                objectInstance.name,
                objectInstance.producingFederateId,
                callbackRoute->second,
                reportRoute == federation->second.serviceReportRoutes.end()
                    ? FederateServiceReportRoute{}
                    : reportRoute->second,
            });
          } catch (...) {
            objectInstance.pendingDiscoveryFederates.erase(receivingFederateId);
            throw;
          }
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
    std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
    for (auto const& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
      if (objectInstance.deleteAccepted) {
        continue;
      }
      for (auto const& [receivingFederateId, knownClassHandle] :
           objectInstance.knownObjectClassHandlesByFederate) {
        static_cast<void>(knownClassHandle);
        if (receivingFederateId == objectInstance.producingFederateId ||
            !federation->second.members.contains(receivingFederateId)) {
          continue;
        }
        auto const callbackRoute =
            federation->second.interactionCallbackRoutes.find(receivingFederateId);
        if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
            !callbackRoute->second) {
          continue;
        }
        for (auto const& [attributeHandle, owner] : objectInstance.attributeOwnersByHandle) {
          static_cast<void>(owner);
          bool const wasInScope = objectAttributeInScope(
              federation->second,
              objectInstance,
              receivingFederateId,
              attributeHandle,
              &previousRegions);
          bool const isInScope = objectAttributeInScope(
              federation->second,
              objectInstance,
              receivingFederateId,
              attributeHandle);
          if (wasInScope == isInScope) {
            continue;
          }
          auto const key = std::make_tuple(
              receivingFederateId,
              objectInstanceHandle,
              isInScope);
          auto position = recipientPositions.find(key);
          if (position == recipientPositions.end()) {
            std::size_t const index = result.recipients.size();
            result.recipients.push_back({
                receivingFederateId,
                objectInstanceHandle,
                {},
                isInScope,
                callbackRoute->second,
            });
            recipientPositions.emplace(key, index);
            position = recipientPositions.find(key);
          }
          result.recipients[position->second].attributeHandles.insert(attributeHandle);
        }
      }
    }
    result.attributeRelevanceAdvisories = attributeRelevanceAdvisoriesForScopeChanges(
        federation->second,
        result.recipients);
    auto transitionAdvisories = attributeRelevanceAdvisoriesForTransitions(
        federation->second,
        0,
        {},
        &previousRegions);
    result.attributeRelevanceAdvisories.insert(
        result.attributeRelevanceAdvisories.end(),
        transitionAdvisories.begin(),
        transitionAdvisories.end());
    return result;
  } catch (...) {
    rollback();
    throw;
  }
}

RegionServiceStatus EmbeddedFederationRegistry::commitRegionModifications(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::uint64_t> const& regionHandles) {
  return commitRegionModificationsWithScopeChanges(
             federationName,
             federateId,
             regionHandles)
      .status;
}

RegionServiceStatus EmbeddedFederationRegistry::deleteRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionServiceStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionServiceStatus::federate_not_member;
  }
  auto region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end()) {
    return RegionServiceStatus::invalid_region;
  }
  if (region->second.ownerFederateId != federateId) {
    return RegionServiceStatus::region_not_created_by_this_federate;
  }
  if (region->second.inUse) {
    return RegionServiceStatus::region_in_use;
  }
  federation->second.regions.erase(region);
  return RegionServiceStatus::applied;
}

RegionDimensionSetResult EmbeddedFederationRegistry::dimensionHandleSetForRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  auto const region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end() ||
      region->second.ownerFederateId != federateId) {
    return {RegionServiceStatus::invalid_region};
  }
  return {RegionServiceStatus::applied, region->second.dimensionHandles};
}

RegionRangeBoundsResult EmbeddedFederationRegistry::rangeBoundsForRegion(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {RegionServiceStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {RegionServiceStatus::federate_not_member};
  }
  auto const region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end() ||
      region->second.ownerFederateId != federateId) {
    return {RegionServiceStatus::invalid_region};
  }
  if (!region->second.dimensionHandles.contains(dimensionHandle)) {
    return {RegionServiceStatus::dimension_not_in_region};
  }

  // A pending value is the value most recently supplied by Set Range Bounds;
  // before a first set, a committed value is the current specification value.
  // A never-initialized dimension has no range and therefore is not a usable
  // region until the caller supplies it and commits the template.
  auto pending = region->second.pendingRangeBounds.find(dimensionHandle);
  if (pending != region->second.pendingRangeBounds.end()) {
    return {RegionServiceStatus::applied, pending->second};
  }
  auto committed = region->second.committedRangeBounds.find(dimensionHandle);
  if (committed != region->second.committedRangeBounds.end()) {
    return {RegionServiceStatus::applied, committed->second};
  }
  return {RegionServiceStatus::invalid_region};
}

RegionServiceStatus EmbeddedFederationRegistry::setRangeBounds(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t regionHandle,
    std::uint64_t dimensionHandle,
    RegionRangeBounds range) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return RegionServiceStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return RegionServiceStatus::federate_not_member;
  }
  if (!federation->second.definition.catalog || !federation->second.dimensionHandles) {
    return RegionServiceStatus::inconsistent_catalog;
  }
  auto region = federation->second.regions.find(regionHandle);
  if (region == federation->second.regions.end()) {
    return RegionServiceStatus::invalid_region;
  }
  if (region->second.ownerFederateId != federateId) {
    return RegionServiceStatus::region_not_created_by_this_federate;
  }
  if (!region->second.dimensionHandles.contains(dimensionHandle)) {
    return RegionServiceStatus::dimension_not_in_region;
  }
  auto const dimensionName = federation->second.dimensionHandles->nameFor(dimensionHandle);
  auto const* dimension = dimensionName
      ? federation->second.definition.catalog->dimension(*dimensionName)
      : nullptr;
  if (dimension == nullptr) {
    return RegionServiceStatus::inconsistent_catalog;
  }
  if (range.lowerBound >= range.upperBound || range.upperBound > dimension->upperBound) {
    return RegionServiceStatus::invalid_range_bound;
  }
  region->second.pendingRangeBounds[dimensionHandle] = range;
  return RegionServiceStatus::applied;
}

bool EmbeddedFederationRegistry::validInteractionClass(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.definition.catalog || !federation.interactionClassHandles) {
    return false;
  }
  auto const name = federation.interactionClassHandles->nameFor(interactionClassHandle);
  return name && federation.definition.catalog->interactionClass(*name) != nullptr;
}

bool EmbeddedFederationRegistry::isReportServiceInvocationInteractionClass(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.interactionClassHandles) {
    return false;
  }
  auto const name = federation.interactionClassHandles->nameFor(interactionClassHandle);
  return name && *name == kReportServiceInvocationInteractionClassName;
}

bool EmbeddedFederationRegistry::hasReportServiceInvocationSubscription(
    Federation const& federation,
    std::uint64_t federateId) {
  if (!federation.interactionClassHandles) {
    return false;
  }
  auto const reportClassHandle = federation.interactionClassHandles->handleFor(
      kReportServiceInvocationInteractionClassName);
  if (!reportClassHandle) {
    return false;
  }
  auto const declarations = federation.interactionDeclarations.find(federateId);
  if (declarations == federation.interactionDeclarations.end()) {
    return false;
  }
  if (declarations->second.subscribedInteractionClasses.contains(*reportClassHandle)) {
    return true;
  }
  auto const regional = declarations->second.regionalSubscribedInteractionClasses.find(
      *reportClassHandle);
  return regional != declarations->second.regionalSubscribedInteractionClasses.end() &&
      !regional->second.empty();
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableInteractionDimensions(
    Federation const& federation,
    std::uint64_t interactionClassHandle) {
  if (!federation.definition.catalog ||
      !federation.interactionClassHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation.interactionClassHandles->nameFor(interactionClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* interactionClass =
        federation.definition.catalog->interactionClass(currentClassName);
    if (interactionClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(
        interactionClass->dimensions.begin(), interactionClass->dimensions.end());
    currentClassName = interactionClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

std::optional<std::set<std::uint64_t>>
EmbeddedFederationRegistry::availableObjectClassDimensions(
    Federation const& federation,
    std::uint64_t objectClassHandle) {
  if (!federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.dimensionHandles) {
    return std::nullopt;
  }
  auto const className = federation.objectClassHandles->nameFor(objectClassHandle);
  if (!className) {
    return std::nullopt;
  }

  std::set<std::string> dimensionNames;
  std::set<std::string> visited;
  std::string currentClassName = *className;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* objectClass = federation.definition.catalog->objectClass(currentClassName);
    if (objectClass == nullptr) {
      return std::nullopt;
    }
    dimensionNames.insert(objectClass->dimensions.begin(), objectClass->dimensions.end());
    currentClassName = objectClass->parentName;
  }

  std::set<std::uint64_t> handles;
  for (std::string const& dimensionName : dimensionNames) {
    if (federation.definition.catalog->dimension(dimensionName) == nullptr) {
      return std::nullopt;
    }
    auto const handle = federation.dimensionHandles->handleFor(dimensionName);
    if (!handle) {
      return std::nullopt;
    }
    handles.insert(*handle);
  }
  return handles;
}

bool EmbeddedFederationRegistry::regionsOverlap(
    Federation const& federation,
    std::uint64_t firstRegionHandle,
    std::uint64_t secondRegionHandle) {
  return regionsOverlap(federation, firstRegionHandle, secondRegionHandle, nullptr);
}

bool EmbeddedFederationRegistry::regionsOverlap(
    Federation const& federation,
    std::uint64_t firstRegionHandle,
    std::uint64_t secondRegionHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides) {
  auto snapshotFor = [&](std::uint64_t regionHandle)
      -> std::optional<RegionSpecificationSnapshot> {
    if (overrides != nullptr) {
      auto const overrideRegion = overrides->find(regionHandle);
      if (overrideRegion != overrides->end()) {
        return overrideRegion->second;
      }
    }
    auto const region = federation.regions.find(regionHandle);
    if (region == federation.regions.end()) {
      return std::nullopt;
    }
    return RegionSpecificationSnapshot{
        region->second.dimensionHandles,
        region->second.committedRangeBounds,
        region->second.specificationCommitted,
    };
  };

  auto const first = snapshotFor(firstRegionHandle);
  auto const second = snapshotFor(secondRegionHandle);
  if (!first || !second ||
      !first->specificationCommitted || !second->specificationCommitted) {
    return false;
  }

  return regionSnapshotsOverlap(federation, *first, *second);
}

bool EmbeddedFederationRegistry::regionSnapshotsOverlap(
    Federation const& federation,
    RegionSpecificationSnapshot const& first,
    RegionSpecificationSnapshot const& second) {
  if (!first.specificationCommitted || !second.specificationCommitted) {
    return false;
  }

  bool sharedDimension = false;
  for (auto const& [dimensionHandle, firstRange] : first.committedRangeBounds) {
    auto const secondRange = second.committedRangeBounds.find(dimensionHandle);
    if (secondRange == second.committedRangeBounds.end()) {
      continue;
    }
    sharedDimension = true;
    // Ranges are half-open [lower, upper).  The strict 2025 definition treats
    // equal lower bounds as overlapping, or otherwise requires ordinary
    // half-open intersection.  When the federation-wide Allow Relaxed DDM
    // switch is enabled, Umbra's documented implementation-specific policy
    // expands only exactly touching committed ranges.  It never removes a
    // strict overlap and does not apply a numerical distance threshold.
    bool const strictOverlap =
        firstRange.lowerBound == secondRange->second.lowerBound ||
        (firstRange.lowerBound < secondRange->second.upperBound &&
         secondRange->second.lowerBound < firstRange.upperBound);
    if (strictOverlap) {
      continue;
    }
    bool const boundaryTouch =
        firstRange.upperBound == secondRange->second.lowerBound ||
        secondRange->second.upperBound == firstRange.lowerBound;
    if (!federation.allowRelaxedDDMSwitch || !boundaryTouch) {
      return false;
    }
  }
  return sharedDimension;
}

bool EmbeddedFederationRegistry::regionOverlapsSnapshot(
    Federation const& federation,
    std::uint64_t regionHandle,
    RegionSpecificationSnapshot const& snapshot) {
  auto const region = federation.regions.find(regionHandle);
  if (region == federation.regions.end() ||
      !snapshot.specificationCommitted ||
      !region->second.specificationCommitted) {
    return false;
  }
  return regionSnapshotsOverlap(
      federation,
      RegionSpecificationSnapshot{
          region->second.dimensionHandles,
          region->second.committedRangeBounds,
          region->second.specificationCommitted,
      },
      snapshot);
}

bool EmbeddedFederationRegistry::regionOverlapsDefault(
    Federation const& federation,
    std::uint64_t regionHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides) {
  std::optional<RegionSpecificationSnapshot> snapshot;
  if (overrides != nullptr) {
    auto const overrideRegion = overrides->find(regionHandle);
    if (overrideRegion != overrides->end()) {
      snapshot = overrideRegion->second;
    }
  }
  if (!snapshot) {
    auto const region = federation.regions.find(regionHandle);
    if (region == federation.regions.end()) {
      return false;
    }
    snapshot = RegionSpecificationSnapshot{
        region->second.dimensionHandles,
        region->second.committedRangeBounds,
        region->second.specificationCommitted,
    };
  }

  // The default region has the full [0, upperBound) range for every FDD
  // dimension. A committed explicit region therefore overlaps it whenever it
  // has at least one dimension. The standard separately defines an empty
  // realization as overlapping no region, including the default region.
  return snapshot->specificationCommitted &&
      !snapshot->dimensionHandles.empty() &&
      snapshot->committedRangeBounds.size() == snapshot->dimensionHandles.size();
}

void EmbeddedFederationRegistry::clearUpdateRegionAssociation(
    Federation::ObjectInstance& objectInstance,
    std::uint64_t attributeHandle) noexcept {
  objectInstance.updateRegionsByAttribute.erase(attributeHandle);
}

void EmbeddedFederationRegistry::promoteDeferredUpdateRegionAssociation(
    Federation::ObjectInstance& objectInstance,
    std::uint64_t federateId,
    std::uint64_t attributeHandle) noexcept {
  auto deferredByAttribute = objectInstance.deferredUpdateRegionsByFederate.find(federateId);
  if (deferredByAttribute == objectInstance.deferredUpdateRegionsByFederate.end()) {
    return;
  }
  auto deferred = deferredByAttribute->second.find(attributeHandle);
  if (deferred == deferredByAttribute->second.end()) {
    return;
  }
  if (!deferred->second.empty()) {
    objectInstance.updateRegionsByAttribute.insert_or_assign(
        attributeHandle,
        deferred->second);
  } else {
    objectInstance.updateRegionsByAttribute.erase(attributeHandle);
  }
  deferredByAttribute->second.erase(deferred);
  if (deferredByAttribute->second.empty()) {
    objectInstance.deferredUpdateRegionsByFederate.erase(deferredByAttribute);
  }
}

void EmbeddedFederationRegistry::refreshRegionUsage(Federation& federation) {
  for (auto& [regionHandle, region] : federation.regions) {
    static_cast<void>(regionHandle);
    region.inUse = false;
  }
  for (auto const& [federateId, declarations] : federation.interactionDeclarations) {
    static_cast<void>(federateId);
    for (auto const& [interactionClassHandle, regions] :
         declarations.regionalSubscribedInteractionClasses) {
      static_cast<void>(interactionClassHandle);
      for (auto const& [regionHandle, active] : regions) {
        static_cast<void>(active);
        auto const region = federation.regions.find(regionHandle);
        if (region != federation.regions.end()) {
          region->second.inUse = true;
        }
      }
    }
  }
  for (auto const& [federateId, declarations] : federation.objectClassAttributeDeclarations) {
    static_cast<void>(federateId);
    for (auto const& [objectClassHandle, perClass] : declarations.byObjectClass) {
      static_cast<void>(objectClassHandle);
      for (auto const& [attributeHandle, regions] : perClass.regionalSubscribedAttributes) {
        static_cast<void>(attributeHandle);
        for (auto const& [regionHandle, active] : regions) {
          static_cast<void>(active);
          auto const region = federation.regions.find(regionHandle);
          if (region != federation.regions.end()) {
            region->second.inUse = true;
          }
        }
      }
    }
  }
  for (auto const& [objectInstanceHandle, objectInstance] : federation.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    for (auto const& [attributeHandle, regions] : objectInstance.updateRegionsByAttribute) {
      static_cast<void>(attributeHandle);
      for (std::uint64_t const regionHandle : regions) {
        auto const region = federation.regions.find(regionHandle);
        if (region != federation.regions.end()) {
          region->second.inUse = true;
        }
      }
    }
    for (auto const& [federateId, deferredByAttribute] :
         objectInstance.deferredUpdateRegionsByFederate) {
      static_cast<void>(federateId);
      for (auto const& [attributeHandle, regions] : deferredByAttribute) {
        static_cast<void>(attributeHandle);
        for (std::uint64_t const regionHandle : regions) {
          auto const region = federation.regions.find(regionHandle);
          if (region != federation.regions.end()) {
            region->second.inUse = true;
          }
        }
      }
    }
  }
}


}  // namespace umbra::detail
