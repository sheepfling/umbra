#include "internal/federation/federation_registry.hpp"
#include "internal/federation/federation_registry_value_helpers.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace umbra::detail {
bool EmbeddedFederationRegistry::objectAttributeInScope(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::uint64_t attributeHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides,
    Federation::ObjectClassAttributeDeclarations const* declarationOverrides) {
  if (objectInstance.deleteAccepted ||
      objectInstance.producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return false;
  }
  auto const receivingMember = federation.members.find(receivingFederateId);
  if (receivingMember == federation.members.end()) {
    return false;
  }
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end()) {
    return false;
  }
  auto const knownClassName = federation.objectClassHandles->nameFor(knownClass->second);
  if (!knownClassName ||
      federation.definition.catalog->objectClass(*knownClassName) == nullptr) {
    return false;
  }
  Federation::ObjectClassAttributeDeclarations const* declarations = declarationOverrides;
  if (declarations == nullptr) {
    auto const currentDeclarations = federation.objectClassAttributeDeclarations.find(
        receivingFederateId);
    if (currentDeclarations == federation.objectClassAttributeDeclarations.end()) {
      return false;
    }
    declarations = &currentDeclarations->second;
  }

  std::set<std::string> visited;
  std::string currentClassName = *knownClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return false;
    }
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            currentClassName,
            attributeHandle)) {
      currentClassName = currentClass->parentName;
      continue;
    }

    auto const perClass = declarations->byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->byObjectClass.end()) {
      auto const regional = perClass->second.regionalSubscribedAttributes.find(
          attributeHandle);
      auto const& updateRegions = associationOverrides == nullptr
          ? objectInstance.updateRegionsByAttribute
          : *associationOverrides;
      auto const associated = updateRegions.find(attributeHandle);
      bool const hasExplicitSubscriptionRegion =
          regional != perClass->second.regionalSubscribedAttributes.end() &&
          !regional->second.empty();
      bool const hasExplicitUpdateRegion =
          associated != updateRegions.end() && !associated->second.empty();

      auto const ordinarySubscription = perClass->second.subscribedAttributes.find(
          attributeHandle);
      if (ordinarySubscription != perClass->second.subscribedAttributes.end() &&
          ordinarySubscription->second) {
        if (!hasExplicitUpdateRegion) {
          return true;
        }
        for (std::uint64_t const associatedRegionHandle : associated->second) {
          if (regionOverlapsDefault(federation, associatedRegionHandle, overrides)) {
            return true;
          }
        }
      }
      if (hasExplicitSubscriptionRegion) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          if (!hasExplicitUpdateRegion) {
            if (regionOverlapsDefault(federation, subscribedRegionHandle, overrides)) {
              return true;
            }
            continue;
          }
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    associatedRegionHandle,
                    overrides)) {
              return true;
            }
          }
        }
      }
    }
    currentClassName = currentClass->parentName;
  }
  return false;
}

bool EmbeddedFederationRegistry::objectAttributeRelevantForAdvisory(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::uint64_t attributeHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* overrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides,
    Federation::ObjectClassAttributeDeclarations const* declarationOverrides) {
  // The enabled policy is exactly the normal in-scope calculation.  Keeping
  // this delegation explicit makes the disabled-policy traversal below easy
  // to audit against §6.1.5 without changing delivery scope itself.
  if (federation.advisoriesUseKnownClassSwitch) {
    return objectAttributeInScope(
        federation,
        objectInstance,
        receivingFederateId,
        attributeHandle,
        overrides,
        associationOverrides,
        declarationOverrides);
  }

  if (objectInstance.deleteAccepted ||
      objectInstance.producingFederateId == receivingFederateId ||
      !federation.members.contains(receivingFederateId) ||
      !federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles) {
    return false;
  }
  // An advisory still concerns a particular known object.  The disabled
  // switch changes which class supplies subscription relevance; it does not
  // authorize an owner-directed callback for an object the receiver has
  // never discovered.
  if (!objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return false;
  }

  Federation::ObjectClassAttributeDeclarations const* declarations =
      declarationOverrides;
  if (declarations == nullptr) {
    auto const currentDeclarations = federation.objectClassAttributeDeclarations.find(
        receivingFederateId);
    if (currentDeclarations == federation.objectClassAttributeDeclarations.end()) {
      return false;
    }
    declarations = &currentDeclarations->second;
  }

  auto const registeredClassName = federation.objectClassHandles->nameFor(
      objectInstance.registeredObjectClassHandle);
  if (!registeredClassName ||
      federation.definition.catalog->objectClass(*registeredClassName) == nullptr) {
    return false;
  }

  auto const& updateRegions = associationOverrides == nullptr
      ? objectInstance.updateRegionsByAttribute
      : *associationOverrides;
  std::set<std::string> visited;
  std::string currentClassName = *registeredClassName;
  while (!currentClassName.empty() && visited.insert(currentClassName).second) {
    auto const* currentClass = federation.definition.catalog->objectClass(currentClassName);
    auto const currentClassHandle = federation.objectClassHandles->handleFor(currentClassName);
    if (currentClass == nullptr || !currentClassHandle) {
      return false;
    }
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            currentClassName,
            attributeHandle)) {
      currentClassName = currentClass->parentName;
      continue;
    }

    auto const perClass = declarations->byObjectClass.find(*currentClassHandle);
    if (perClass != declarations->byObjectClass.end()) {
      auto const regional = perClass->second.regionalSubscribedAttributes.find(
          attributeHandle);
      auto const associated = updateRegions.find(attributeHandle);
      bool const hasExplicitSubscriptionRegion =
          regional != perClass->second.regionalSubscribedAttributes.end() &&
          !regional->second.empty();
      bool const hasExplicitUpdateRegion =
          associated != updateRegions.end() && !associated->second.empty();

      auto const ordinarySubscription = perClass->second.subscribedAttributes.find(
          attributeHandle);
      if (ordinarySubscription != perClass->second.subscribedAttributes.end() &&
          ordinarySubscription->second) {
        if (!hasExplicitUpdateRegion) {
          return true;
        }
        for (std::uint64_t const associatedRegionHandle : associated->second) {
          if (regionOverlapsDefault(federation, associatedRegionHandle, overrides)) {
            return true;
          }
        }
      }

      if (hasExplicitSubscriptionRegion) {
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          if (!hasExplicitUpdateRegion) {
            if (regionOverlapsDefault(federation, subscribedRegionHandle, overrides)) {
              return true;
            }
            continue;
          }
          for (std::uint64_t const associatedRegionHandle : associated->second) {
            if (regionsOverlap(
                    federation,
                    subscribedRegionHandle,
                    associatedRegionHandle,
                    overrides)) {
              return true;
            }
          }
        }
      }
    }
    currentClassName = currentClass->parentName;
  }
  return false;
}

EmbeddedFederationRegistry::AttributeRelevanceRateSnapshot
EmbeddedFederationRegistry::attributeRelevanceRateSnapshotForState(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t declarationOverrideFederateId,
    std::uint64_t attributeHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides,
    Federation::ObjectClassAttributeDeclarations const* declarationOverrides) {
  AttributeRelevanceRateSnapshot result;
  bool hasUnspecifiedDefault = false;
  std::optional<std::pair<double, std::string>> maximumExplicitRate;

  for (auto const& [receivingFederateId, knownClassHandle] :
       objectInstance.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownClassHandle);
    if (receivingFederateId == objectInstance.producingFederateId ||
        !federation.members.contains(receivingFederateId)) {
      continue;
    }

    Federation::ObjectClassAttributeDeclarations const* receiverDeclarations = nullptr;
    if (declarationOverrideFederateId == receivingFederateId &&
        declarationOverrides != nullptr) {
      receiverDeclarations = declarationOverrides;
    } else {
      auto const currentDeclarations = federation.objectClassAttributeDeclarations.find(
          receivingFederateId);
      if (currentDeclarations == federation.objectClassAttributeDeclarations.end()) {
        continue;
      }
      receiverDeclarations = &currentDeclarations->second;
    }

    if (!objectAttributeRelevantForAdvisory(
            federation,
            objectInstance,
            receivingFederateId,
            attributeHandle,
            regionOverrides,
            associationOverrides,
            receiverDeclarations)) {
      continue;
    }
    result.relevant = true;

    auto const designator = subscribedUpdateRateDesignatorForAttribute(
        federation,
        objectInstance,
        receivingFederateId,
        attributeHandle,
        regionOverrides,
        associationOverrides,
        receiverDeclarations);
    if (!designator) {
      hasUnspecifiedDefault = true;
      continue;
    }
    auto const value = updateRateValueForNormalizedDesignator(
        *federation.definition.catalog,
        *designator);
    if (!value) {
      continue;
    }
    if (!maximumExplicitRate ||
        *value > maximumExplicitRate->first ||
        (*value == maximumExplicitRate->first &&
         *designator < maximumExplicitRate->second)) {
      maximumExplicitRate = std::make_pair(*value, *designator);
    }
  }

  if (!hasUnspecifiedDefault && maximumExplicitRate) {
    result.updateRateDesignator = maximumExplicitRate->second;
  }
  return result;
}

std::optional<std::string>
EmbeddedFederationRegistry::subscribedUpdateRateDesignatorForAttribute(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::uint64_t receivingFederateId,
    std::uint64_t attributeHandle,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* regionOverrides,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* associationOverrides,
    Federation::ObjectClassAttributeDeclarations const* declarationOverrides) {
  if (!federation.definition.catalog ||
      !federation.objectClassHandles ||
      !federation.attributeHandles ||
      !federation.members.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const knownClass = objectInstance.knownObjectClassHandlesByFederate.find(
      receivingFederateId);
  if (knownClass == objectInstance.knownObjectClassHandlesByFederate.end()) {
    return std::nullopt;
  }
  Federation::ObjectClassAttributeDeclarations const* declarations = declarationOverrides;
  if (declarations == nullptr) {
    auto const currentDeclarations = federation.objectClassAttributeDeclarations.find(
        receivingFederateId);
    if (currentDeclarations == federation.objectClassAttributeDeclarations.end()) {
      return std::nullopt;
    }
    declarations = &currentDeclarations->second;
  }

  // Turn Updates On carries the maximum actively subscribed rate for the
  // attribute.  Accumulate every applicable active declaration in the
  // receiver's class lineage; an omitted/default declaration suppresses the
  // optional designator even when another declaration supplies an explicit
  // rate.
  bool hasUnspecifiedDefault = false;
  std::optional<std::pair<double, std::string>> maximumExplicitRate;
  auto considerDesignator = [&](std::string const& designator) {
    if (designator.empty()) {
      hasUnspecifiedDefault = true;
      return;
    }
    auto const value = updateRateValueForNormalizedDesignator(
        *federation.definition.catalog,
        designator);
    if (!value) {
      return;
    }
    if (!maximumExplicitRate ||
        *value > maximumExplicitRate->first ||
        (*value == maximumExplicitRate->first &&
         designator < maximumExplicitRate->second)) {
      maximumExplicitRate = std::make_pair(*value, designator);
    }
  };

  auto const& updateRegions = associationOverrides == nullptr
      ? objectInstance.updateRegionsByAttribute
      : *associationOverrides;
  std::set<std::uint64_t> visited;
  // Rate selection follows the same static policy as relevance itself.  With
  // Advisories Use Known Class disabled, a subclass-only subscription remains
  // visible even when the receiving federate knows only an ancestor.
  std::uint64_t currentClassHandle = federation.advisoriesUseKnownClassSwitch
      ? knownClass->second
      : objectInstance.registeredObjectClassHandle;
  while (visited.insert(currentClassHandle).second) {
    auto const currentClassName = federation.objectClassHandles->nameFor(currentClassHandle);
    if (!currentClassName) {
      return std::nullopt;
    }
    auto const* currentClass = federation.definition.catalog->objectClass(*currentClassName);
    if (currentClass == nullptr) {
      return std::nullopt;
    }
    if (!federation.attributeHandles->nameFor(
            federation.definition.catalog.get(),
            *currentClassName,
            attributeHandle)) {
      if (currentClass->parentName.empty()) {
        break;
      }
      auto const parentHandle = federation.objectClassHandles->handleFor(
          currentClass->parentName);
      if (!parentHandle) {
        break;
      }
      currentClassHandle = *parentHandle;
      continue;
    }

    auto const perClass = declarations->byObjectClass.find(currentClassHandle);
    if (perClass != declarations->byObjectClass.end()) {
      auto const regional = perClass->second.regionalSubscribedAttributes.find(
          attributeHandle);
      auto const associated = updateRegions.find(attributeHandle);
      bool const hasExplicitSubscriptionRegion =
          regional != perClass->second.regionalSubscribedAttributes.end() &&
          !regional->second.empty();
      bool const hasExplicitUpdateRegion =
          associated != updateRegions.end() && !associated->second.empty();

      // Ordinary and regional declarations are independent.  Include the
      // ordinary default-region rate whenever that declaration is active, and
      // then include any active overlapping regional rates.
      auto const ordinary = perClass->second.subscribedAttributes.find(
          attributeHandle);
      if (ordinary != perClass->second.subscribedAttributes.end() &&
          ordinary->second &&
          (!hasExplicitUpdateRegion || [&] {
            for (std::uint64_t const associatedRegionHandle : associated->second) {
              if (regionOverlapsDefault(federation, associatedRegionHandle, regionOverrides)) {
                return true;
              }
            }
            return false;
          }())) {
        auto const designator = perClass->second.subscribedUpdateRateDesignators.find(
            attributeHandle);
        considerDesignator(
            designator == perClass->second.subscribedUpdateRateDesignators.end()
                ? std::string{}
                : designator->second);
      }
      if (hasExplicitSubscriptionRegion) {
        auto const regionalDesignators =
            perClass->second.regionalSubscribedUpdateRateDesignators.find(
                attributeHandle);
        for (auto const& [subscribedRegionHandle, active] : regional->second) {
          if (!active) {
            continue;
          }
          bool overlaps = false;
          if (!hasExplicitUpdateRegion) {
            overlaps = regionOverlapsDefault(
                federation,
                subscribedRegionHandle,
                regionOverrides);
          } else {
            for (std::uint64_t const associatedRegionHandle : associated->second) {
              if (regionsOverlap(
                      federation,
                      subscribedRegionHandle,
                      associatedRegionHandle,
                      regionOverrides)) {
                overlaps = true;
                break;
              }
            }
          }
          if (!overlaps) {
            continue;
          }
          std::string designator;
          if (regionalDesignators !=
              perClass->second.regionalSubscribedUpdateRateDesignators.end()) {
            auto const stored = regionalDesignators->second.find(subscribedRegionHandle);
            if (stored != regionalDesignators->second.end()) {
              designator = stored->second;
            }
          }
          considerDesignator(designator);
        }
      }
    }

    if (currentClass->parentName.empty()) {
      break;
    }
    auto const parentHandle = federation.objectClassHandles->handleFor(
        currentClass->parentName);
    if (!parentHandle) {
      break;
    }
    currentClassHandle = *parentHandle;
  }

  if (hasUnspecifiedDefault || !maximumExplicitRate) {
    return std::nullopt;
  }
  return maximumExplicitRate->second;
}

std::vector<ObjectInstanceScopeChangeRecipient>
EmbeddedFederationRegistry::objectInstanceScopeChangesForAssociation(
    Federation const& federation,
    Federation::ObjectInstance const& objectInstance,
    std::set<std::uint64_t> const& attributeHandles,
    std::map<std::uint64_t, std::set<std::uint64_t>> const& associationOverrides) {
  std::vector<ObjectInstanceScopeChangeRecipient> result;
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
  for (auto const& [receivingFederateId, knownClassHandle] :
       objectInstance.knownObjectClassHandlesByFederate) {
    static_cast<void>(knownClassHandle);
    if (receivingFederateId == objectInstance.producingFederateId ||
        !federation.members.contains(receivingFederateId)) {
      continue;
    }
    auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
    if (callbackRoute == federation.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      bool const wasInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle);
      bool const isInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle,
          nullptr,
          &associationOverrides);
      if (wasInScope == isInScope) {
        continue;
      }

      auto const key = std::make_tuple(
          receivingFederateId,
          objectInstance.handle,
          isInScope);
      auto position = recipientPositions.find(key);
      if (position == recipientPositions.end()) {
        std::size_t const index = result.size();
        result.push_back({
            receivingFederateId,
            objectInstance.handle,
            {},
            isInScope,
            callbackRoute->second,
        });
        recipientPositions.emplace(key, index);
        position = recipientPositions.find(key);
      }
      result[position->second].attributeHandles.insert(attributeHandle);
    }
  }
  return result;
}

std::vector<ObjectInstanceScopeChangeRecipient>
EmbeddedFederationRegistry::objectInstanceScopeChangesForSubscription(
    Federation const& federation,
    std::uint64_t receivingFederateId,
    std::set<std::uint64_t> const& attributeHandles,
    Federation::ObjectClassAttributeDeclarations const& previousDeclarations) {
  std::vector<ObjectInstanceScopeChangeRecipient> result;
  std::map<std::tuple<std::uint64_t, std::uint64_t, bool>, std::size_t> recipientPositions;
  if (!federation.members.contains(receivingFederateId)) {
    return result;
  }
  auto const callbackRoute = federation.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return result;
  }

  for (auto const& [objectInstanceHandle, objectInstance] : federation.objectInstances) {
    static_cast<void>(objectInstanceHandle);
    if (objectInstance.producingFederateId == receivingFederateId ||
        !objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      bool const wasInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle,
          nullptr,
          nullptr,
          &previousDeclarations);
      bool const isInScope = objectAttributeInScope(
          federation,
          objectInstance,
          receivingFederateId,
          attributeHandle);
      // §10.1.3 treats loss of scope caused by the joined federate's own
      // unsubscribe as implicit out-of-scope: do not advise that same
      // federate when its last applicable subscription is removed. An In
      // transition caused by a new subscription is still advisory.
      if (wasInScope == isInScope || !isInScope) {
        continue;
      }

      auto const key = std::make_tuple(
          receivingFederateId,
          objectInstance.handle,
          isInScope);
      auto position = recipientPositions.find(key);
      if (position == recipientPositions.end()) {
        std::size_t const index = result.size();
        result.push_back({
            receivingFederateId,
            objectInstance.handle,
            {},
            isInScope,
            callbackRoute->second,
        });
        recipientPositions.emplace(key, index);
        position = recipientPositions.find(key);
      }
      result[position->second].attributeHandles.insert(attributeHandle);
    }
  }
  return result;
}

std::vector<AttributeRelevanceAdvisoryRecipient>
EmbeddedFederationRegistry::attributeRelevanceAdvisoriesForScopeChanges(
    Federation const& federation,
    std::vector<ObjectInstanceScopeChangeRecipient> const& scopeChanges) {
  // The mutation-specific transition planner below compares the complete
  // pre/post state across all known receivers. Keeping this legacy helper
  // empty prevents duplicate owner callbacks when another receiver already
  // keeps an attribute relevant. `scopeChanges` remains the independent
  // Attribute Scope lane.
  static_cast<void>(federation);
  static_cast<void>(scopeChanges);
  return {};
}

std::vector<AttributeRelevanceAdvisoryRecipient>
EmbeddedFederationRegistry::attributeRelevanceAdvisoriesForTransitions(
    Federation const& federation,
    std::uint64_t receivingFederateId,
    std::set<std::uint64_t> const& attributeHandles,
    std::map<std::uint64_t, RegionSpecificationSnapshot> const* previousRegions,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* previousAssociations,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* currentAssociations,
    Federation::ObjectClassAttributeDeclarations const* previousDeclarations,
    std::optional<std::uint64_t> objectInstanceHandle) {
  std::vector<AttributeRelevanceAdvisoryRecipient> result;
  if (receivingFederateId != 0 &&
      !federation.members.contains(receivingFederateId)) {
    return result;
  }

  std::map<
      std::tuple<
          std::uint64_t,
          std::uint64_t,
          std::uint64_t,
          bool,
          std::optional<std::string>>,
      std::size_t>
      recipientPositions;
  auto appendTransition = [&](std::uint64_t instanceHandle,
                              Federation::ObjectInstance const& objectInstance,
                              std::uint64_t attributeHandle) {
    if (objectInstance.deleteAccepted) {
      return;
    }
    auto const owner = objectInstance.attributeOwnersByHandle.find(attributeHandle);
    if (owner == objectInstance.attributeOwnersByHandle.end() ||
        owner->second == 0 ||
        !federation.members.contains(owner->second)) {
      return;
    }

    auto const previousState = attributeRelevanceRateSnapshotForState(
        federation,
        objectInstance,
        receivingFederateId,
        attributeHandle,
        previousRegions,
        previousAssociations,
        previousDeclarations);
    auto const currentState = attributeRelevanceRateSnapshotForState(
        federation,
        objectInstance,
        receivingFederateId,
        attributeHandle,
        nullptr,
        currentAssociations);
    bool const relevanceChanged = previousState.relevant != currentState.relevant;
    bool const updateRateChanged = previousState.relevant && currentState.relevant &&
        previousState.updateRateDesignator != currentState.updateRateDesignator;
    // The complete pre/post comparison owns both relevance and rate
    // transitions, including advisory-only changes that do not alter
    // Attribute Scope.
    if (!relevanceChanged && !updateRateChanged) {
      return;
    }

    auto const callbackRoute = federation.interactionCallbackRoutes.find(owner->second);
    if (callbackRoute == federation.interactionCallbackRoutes.end() ||
        !callbackRoute->second) {
      return;
    }
    auto const updateRateDesignator = currentState.relevant
        ? currentState.updateRateDesignator
        : std::nullopt;
    auto const key = std::make_tuple(
        owner->second,
        std::uint64_t{0},
        instanceHandle,
        currentState.relevant,
        updateRateDesignator);
    auto position = recipientPositions.find(key);
    if (position == recipientPositions.end()) {
      std::size_t const index = result.size();
      result.push_back({
          owner->second,
          std::uint64_t{0},
          instanceHandle,
          {},
          currentState.relevant,
          updateRateDesignator,
          callbackRoute->second,
      });
      recipientPositions.emplace(key, index);
      position = recipientPositions.find(key);
    }
    result[position->second].attributeHandles.insert(attributeHandle);
  };

  for (auto const& [instanceHandle, objectInstance] : federation.objectInstances) {
    if (objectInstanceHandle && *objectInstanceHandle != instanceHandle) {
      continue;
    }
    if (attributeHandles.empty()) {
      for (auto const& [attributeHandle, owner] : objectInstance.attributeOwnersByHandle) {
        static_cast<void>(owner);
        appendTransition(instanceHandle, objectInstance, attributeHandle);
      }
      continue;
    }
    for (std::uint64_t const attributeHandle : attributeHandles) {
      appendTransition(instanceHandle, objectInstance, attributeHandle);
    }
  }
  return result;
}

}  // namespace umbra::detail
