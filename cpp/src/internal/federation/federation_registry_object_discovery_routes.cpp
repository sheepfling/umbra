#include "internal/federation/federation_registry.hpp"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace umbra::detail {
std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planJoinedFederateMomObjectDiscoveriesForInstance(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  auto object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
      objectInstanceHandle);
  if (object == federation->second.rtiOwnedJoinedFederateMomObjects.end() ||
      (!object->second.federationExecutionObject &&
       !federation->second.members.contains(object->second.joinedFederateId))) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.members.size());
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (object->second.knownFederateIds.contains(receivingFederateId) ||
          object->second.pendingDiscoveryFederateIds.contains(receivingFederateId) ||
          !candidateJoinedFederateMomObjectDiscoveryClass(
              federation->second,
              object->second,
              receivingFederateId)) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
          receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      auto const [pending, insertedPending] =
          object->second.pendingDiscoveryFederateIds.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            object->second.objectInstanceHandle,
            object->second.objectClassHandle,
            object->second.federationExecutionObject
                ? L"HLAfederation"
                : L"HLAfederate-" + std::to_wstring(object->second.joinedFederateId),
            0U,
            callbackRoute->second,
            FederateServiceReportRoute{},
            true,
        });
      } catch (...) {
        object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      object->second.pendingDiscoveryFederateIds.erase(recipient.receivingFederateId);
    }
    throw;
  }
  return result;
}

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planJoinedFederateMomObjectDiscoveriesForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(
      receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.rtiOwnedJoinedFederateMomObjects.size());
  try {
    for (auto& [objectInstanceHandle, object] :
         federation->second.rtiOwnedJoinedFederateMomObjects) {
      static_cast<void>(objectInstanceHandle);
      if (object.knownFederateIds.contains(receivingFederateId) ||
          object.pendingDiscoveryFederateIds.contains(receivingFederateId) ||
          (!object.federationExecutionObject &&
           !federation->second.members.contains(object.joinedFederateId)) ||
          !candidateJoinedFederateMomObjectDiscoveryClass(
              federation->second,
              object,
              receivingFederateId)) {
        continue;
      }
      auto const [pending, insertedPending] =
          object.pendingDiscoveryFederateIds.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
            receivingFederateId,
            object.objectInstanceHandle,
            object.objectClassHandle,
            object.federationExecutionObject
                ? L"HLAfederation"
                : L"HLAfederate-" + std::to_wstring(object.joinedFederateId),
            0U,
            callbackRoute->second,
            FederateServiceReportRoute{},
            true,
        });
      } catch (...) {
        object.pendingDiscoveryFederateIds.erase(receivingFederateId);
        throw;
      }
    }
  } catch (...) {
    for (auto const& recipient : result) {
      auto const object = federation->second.rtiOwnedJoinedFederateMomObjects.find(
          recipient.objectInstanceHandle);
      if (object != federation->second.rtiOwnedJoinedFederateMomObjects.end()) {
        object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
      }
    }
    throw;
  }
  return result;
}

void EmbeddedFederationRegistry::cancelObjectInstanceDiscovery(
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
  instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
}

void EmbeddedFederationRegistry::cancelJoinedFederateMomObjectDiscovery(
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
  object->second.pendingDiscoveryFederateIds.erase(receivingFederateId);
}
}  // namespace umbra::detail
