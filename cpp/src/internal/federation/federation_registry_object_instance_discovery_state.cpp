#include "internal/federation/federation_registry.hpp"

#include <cstdint>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>

namespace umbra::detail {
std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginObjectInstanceDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation("beginObjectInstanceDiscovery");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end()) {
    return std::nullopt;
  }

  // A route is single-use. Releasing the reservation before every recheck
  // permits a later declaration change to plan a fresh callback if this one
  // has become ineligible.
  instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
  if (instance->second.deleteAccepted ||
      !federation->second.members.contains(receivingFederateId) ||
      instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
      federation->second,
      instance->second,
      receivingFederateId);
  if (!discoveredClass) {
    return std::nullopt;
  }

  auto const [knownClass, insertedKnownClass] =
      instance->second.knownObjectClassHandlesByFederate.emplace(
          receivingFederateId,
          *discoveredClass);
  static_cast<void>(knownClass);
  if (!insertedKnownClass) {
    return std::nullopt;
  }
  auto const receivingMember = federation->second.members.find(receivingFederateId);
  if (receivingMember != federation->second.members.end() &&
      receivingMember->second.successfulObjectInstanceDiscoveriesCount !=
          std::numeric_limits<std::uint64_t>::max()) {
    ++receivingMember->second.successfulObjectInstanceDiscoveriesCount;
  }
  return knownObjectInstanceSnapshot(federation->second, instance->second, receivingFederateId);
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::beginJoinedFederateMomObjectDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  auto instrumentationScope = beginInstrumentation(
      "beginJoinedFederateMomObjectDiscovery");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return std::nullopt;
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
    return std::nullopt;
  }

  object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
  if (!federation->second.members.contains(receivingFederateId) ||
      object->second.knownFederateIds.contains(receivingFederateId)) {
    return std::nullopt;
  }
  auto const discoveredClass = candidateJoinedFederateMomObjectDiscoveryClass(
      federation->second,
      object->second,
      receivingFederateId);
  if (!discoveredClass) {
    return std::nullopt;
  }
  object->second.knownFederateIds.insert(receivingFederateId);
  std::set<std::uint64_t> initialAttributeHandles;
  for (auto const& [attributeHandle, value] : object->second.initialAttributeValues) {
    static_cast<void>(value);
    initialAttributeHandles.insert(attributeHandle);
  }
  return KnownObjectInstanceSnapshot{
      object->second.objectInstanceHandle,
      *discoveredClass,
      object->second.federationExecutionObject
          ? L"HLAfederation"
          : L"HLAfederate-" + std::to_wstring(object->second.joinedFederateId),
      0U,
      std::move(initialAttributeHandles),
  };
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::knownObjectInstanceFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance != federation->second.objectInstances.end()) {
    return knownObjectInstanceSnapshot(federation->second, instance->second, federateId);
  }
  // RTI-owned MOM objects are deliberately absent from objectInstances, but
  // once public discovery has promoted one they are valid inputs to the
  // support lookup services just like a federate-created instance.  Keep the
  // known-instance check recipient-local so an undiscovered MOM handle cannot
  // be used to infer its name or class.
  auto const momObject = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (momObject == federation->second.rtiOwnedJoinedFederateMomObjects.end() ||
      !momObject->second.knownFederateIds.contains(federateId)) {
    return std::nullopt;
  }
  std::set<std::uint64_t> initialAttributeHandles;
  for (auto const& [attributeHandle, value] : momObject->second.initialAttributeValues) {
    static_cast<void>(value);
    initialAttributeHandles.insert(attributeHandle);
  }
  return KnownObjectInstanceSnapshot{
      momObject->second.objectInstanceHandle,
      momObject->second.objectClassHandle,
      momObject->second.federationExecutionObject
          ? L"HLAfederation"
          : L"HLAfederate-" + std::to_wstring(momObject->second.joinedFederateId),
      0U,
      std::move(initialAttributeHandles),
  };
}

std::optional<KnownObjectInstanceSnapshot>
EmbeddedFederationRegistry::knownObjectInstanceByNameFor(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(federateId)) {
    return std::nullopt;
  }
  auto const handle = federation->second.objectInstanceHandlesByName.find(objectInstanceName);
  if (handle != federation->second.objectInstanceHandlesByName.end()) {
    auto const instance = federation->second.objectInstances.find(handle->second);
    if (instance != federation->second.objectInstances.end()) {
      return knownObjectInstanceSnapshot(federation->second, instance->second, federateId);
    }
  }

  // HLAfederation and HLAfederate-* are RTI-owned names.  They are not
  // inserted into the federate-created name directory because that directory
  // also owns reservation/collision semantics; resolve them only through the
  // recipient's MOM known-instance ledger.
  for (auto const& [objectInstanceHandle, object] :
       federation->second.rtiOwnedJoinedFederateMomObjects) {
    auto const expectedName = object.federationExecutionObject
        ? L"HLAfederation"
        : L"HLAfederate-" + std::to_wstring(object.joinedFederateId);
    if (expectedName != objectInstanceName ||
        !object.knownFederateIds.contains(federateId)) {
      continue;
    }
    std::set<std::uint64_t> initialAttributeHandles;
    for (auto const& [attributeHandle, value] : object.initialAttributeValues) {
      static_cast<void>(value);
      initialAttributeHandles.insert(attributeHandle);
    }
    return KnownObjectInstanceSnapshot{
        objectInstanceHandle,
        object.objectClassHandle,
        expectedName,
        0U,
        std::move(initialAttributeHandles),
    };
  }
  return std::nullopt;
}

}  // namespace umbra::detail
