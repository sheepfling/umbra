#include "internal/federation/federation_registry.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <string>

namespace umbra::detail {
std::set<std::uint64_t> EmbeddedFederationRegistry::objectInstanceScopeAttributes(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle,
    std::set<std::uint64_t> const& attributeHandles,
    bool expectedInScope) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const receivingMember = federation->second.members.find(receivingFederateId);
  if (receivingMember == federation->second.members.end() ||
      !receivingMember->second.attributeScopeAdvisorySwitch) {
    return {};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return {};
  }

  std::set<std::uint64_t> result;
  for (std::uint64_t const attributeHandle : attributeHandles) {
    if (objectAttributeInScope(
            federation->second,
            instance->second,
            receivingFederateId,
            attributeHandle) == expectedInScope) {
      result.insert(attributeHandle);
    }
  }
  return result;
}

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation("beginObjectInstanceRemoval");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || !instance->second.deleteAccepted) {
    return std::nullopt;
  }
  if (!instance->second.pendingRemovalFederates.contains(receivingFederateId)) {
    return std::nullopt;
  }

  // A forced connection-loss resign may have queued an ordinary automatic
  // removal before this recipient later opens its TSO boundary. Do not let
  // that receive-order work erase the object's known state while a marked
  // at-or-before-cutoff update or directed interaction still requires it.
  // Keep the original removal reservation; the matching TSO completion will
  // replan this callback after its Time Advance Grant.
  if (instance->second.connectionLossAutomaticRemovalFederates.contains(
          receivingFederateId) &&
      hasPendingConnectionLossTsoObjectDelivery(
          federation->second,
          objectInstanceHandle,
          receivingFederateId)) {
    instance->second.deferredConnectionLossTsoRemovalFederates.insert(
        receivingFederateId);
    return std::nullopt;
  }

  instance->second.pendingRemovalFederates.erase(receivingFederateId);
  instance->second.connectionLossAutomaticRemovalFederates.erase(receivingFederateId);
  instance->second.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
  auto const known = instance->second.knownObjectClassHandlesByFederate.find(receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      known == instance->second.knownObjectClassHandlesByFederate.end()) {
    if (canPurgeDeletedObjectInstance(instance->second)) {
      federation->second.objectInstanceHandlesByName.erase(instance->second.name);
      federation->second.objectInstances.erase(instance);
      refreshRegionUsage(federation->second);
    }
    return std::nullopt;
  }

  auto const receivingMember = federation->second.members.find(receivingFederateId);
  if (receivingMember != federation->second.members.end() &&
      receivingMember->second.successfulObjectInstanceRemovalsCount !=
          std::numeric_limits<std::uint64_t>::max()) {
    ++receivingMember->second.successfulObjectInstanceRemovalsCount;
  }

  RemovedObjectInstanceSnapshot const result{
      instance->second.handle,
      instance->second.producingFederateId,
  };
  instance->second.knownObjectClassHandlesByFederate.erase(known);
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
    refreshRegionUsage(federation->second);
  }
  return result;
}

std::optional<RemovedObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginJoinedFederateMomObjectRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation(
      "beginJoinedFederateMomObjectRemoval");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end() ||
      !object->second.pendingRemovalFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  object->second.pendingRemovalFederateIds.erase(receivingFederateId);
  if (!object->second.knownFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  if (!federation->second.members.contains(receivingFederateId)) {
    object->second.knownFederateIds.erase(receivingFederateId);
    return std::nullopt;
  }
  object->second.knownFederateIds.erase(receivingFederateId);
  RemovedObjectInstanceSnapshot const result{object->second.objectInstanceHandle, 0U};
  if (object->second.knownFederateIds.empty() &&
      object->second.pendingRemovalFederateIds.empty()) {
    clearPendingAttributeOwnershipQueries(
        federation->second,
        object->second.objectInstanceHandle);
    federation->second.rtiOwnedJoinedFederateMomObjects.erase(object);
  }
  return result;
}

void EmbeddedFederationRegistry::cancelObjectInstanceRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return;
  }
  instance->second.pendingRemovalFederates.erase(receivingFederateId);
  instance->second.connectionLossAutomaticRemovalFederates.erase(receivingFederateId);
  instance->second.deferredConnectionLossTsoRemovalFederates.erase(receivingFederateId);
  if (canPurgeDeletedObjectInstance(instance->second)) {
    federation->second.objectInstanceHandlesByName.erase(instance->second.name);
    federation->second.objectInstances.erase(instance);
    refreshRegionUsage(federation->second);
  }
}

void EmbeddedFederationRegistry::cancelJoinedFederateMomObjectRemoval(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return;
  }
  object->second.pendingRemovalFederateIds.erase(receivingFederateId);
}

}  // namespace umbra::detail
