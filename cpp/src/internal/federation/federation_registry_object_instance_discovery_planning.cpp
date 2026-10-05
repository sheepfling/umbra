#include "internal/federation/federation_registry.hpp"

#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planObjectInstanceDiscoveriesForInstance(
    std::wstring const& federationName,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {};
  }
  auto instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() || instance->second.deleteAccepted) {
    return {};
  }

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.members.size());
  try {
    for (auto const& [receivingFederateId, membership] : federation->second.members) {
      static_cast<void>(membership);
      if (instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          instance->second.pendingDiscoveryFederates.contains(receivingFederateId)) {
        continue;
      }
      auto const discoveredClass = candidateObjectInstanceDiscoveryClass(
          federation->second,
          instance->second,
          receivingFederateId);
      if (!discoveredClass) {
        continue;
      }
      auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
      if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
          !callbackRoute->second) {
        continue;
      }
      auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

      auto const [pending, insertedPending] =
          instance->second.pendingDiscoveryFederates.insert(receivingFederateId);
      static_cast<void>(pending);
      if (!insertedPending) {
        continue;
      }
      try {
        result.push_back({
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
    for (auto const& recipient : result) {
      instance->second.pendingDiscoveryFederates.erase(recipient.receivingFederateId);
    }
    throw;
  }
  return result;
}

std::vector<ObjectInstanceDiscoveryRecipient>
EmbeddedFederationRegistry::planObjectInstanceDiscoveriesForFederate(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(receivingFederateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    return {};
  }
  auto const reportRoute = federation->second.serviceReportRoutes.find(receivingFederateId);

  std::vector<ObjectInstanceDiscoveryRecipient> result;
  result.reserve(federation->second.objectInstances.size());
  try {
    for (auto& [objectInstanceHandle, objectInstance] : federation->second.objectInstances) {
      static_cast<void>(objectInstanceHandle);
      if (objectInstance.deleteAccepted ||
          objectInstance.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
          objectInstance.pendingDiscoveryFederates.contains(receivingFederateId)) {
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
      try {
        result.push_back({
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
  } catch (...) {
    for (auto const& recipient : result) {
      auto const instance = federation->second.objectInstances.find(
          recipient.objectInstanceHandle);
      if (instance != federation->second.objectInstances.end()) {
        instance->second.pendingDiscoveryFederates.erase(receivingFederateId);
      }
    }
    throw;
  }
  return result;
}

std::vector<ObjectInstanceInitialAttributeReflection>
EmbeddedFederationRegistry::planInitialObjectInstanceAttributeReflectionsForDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) const {
  std::scoped_lock lock(mutex_);
  auto const federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId) ||
      instance->second.producingFederateId == receivingFederateId ||
      !federation->second.definition.catalog ||
      !federation->second.objectClassHandles ||
      !federation->second.attributeHandles) {
    return {};
  }

  std::vector<std::uint64_t> availableAttributeHandles;
  availableAttributeHandles.reserve(instance->second.attributeValues.size());
  for (auto const& [attributeHandle, value] : instance->second.attributeValues) {
    static_cast<void>(value);
    availableAttributeHandles.push_back(attributeHandle);
  }
  if (availableAttributeHandles.empty()) {
    return {};
  }

  auto const recipient = candidateReceiveOrderAttributeUpdateRecipient(
      federation->second,
      instance->second.producingFederateId,
      receivingFederateId,
      objectInstanceHandle,
      availableAttributeHandles,
      nullptr);
  if (!recipient || recipient->receivedAttributeHandles.empty()) {
    return {};
  }

  std::map<std::string, std::map<std::uint64_t, rti1516_2025::VariableLengthData>>
      valuesByTransportation;
  for (std::uint64_t const attributeHandle : recipient->receivedAttributeHandles) {
    auto const value = instance->second.attributeValues.find(attributeHandle);
    if (value == instance->second.attributeValues.end()) {
      continue;
    }
    auto const transportationName = effectiveAttributeTransportationName(
        federation->second,
        instance->second,
        attributeHandle);
    if (!transportationName) {
      return {};
    }
    valuesByTransportation[*transportationName].insert_or_assign(
        attributeHandle, value->second);
  }

  std::vector<ObjectInstanceInitialAttributeReflection> result;
  result.reserve(valuesByTransportation.size());
  for (auto& [transportationName, attributeValues] : valuesByTransportation) {
    if (!attributeValues.empty()) {
      result.push_back({transportationName, std::move(attributeValues)});
    }
  }
  return result;
}

std::vector<AttributeRelevanceAdvisoryRecipient>
EmbeddedFederationRegistry::planInitialAttributeRelevanceAdvisoriesForDiscovery(
    std::wstring const& federationName,
    std::uint64_t receivingFederateId,
    std::uint64_t objectInstanceHandle) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end() ||
      !federation->second.members.contains(receivingFederateId)) {
    return {};
  }
  auto const instance = federation->second.objectInstances.find(objectInstanceHandle);
  if (instance == federation->second.objectInstances.end() ||
      instance->second.deleteAccepted ||
      !instance->second.knownObjectClassHandlesByFederate.contains(receivingFederateId)) {
    return {};
  }

  // Registration/discovery has no prior receiver declaration or source-region
  // association.  Supplying empty snapshots to the common transition planner
  // keeps initial relevance separate from later mutation transitions while
  // preserving the same rate aggregation and owner callback grouping.
  Federation::ObjectClassAttributeDeclarations previousDeclarations;
  std::map<std::uint64_t, std::set<std::uint64_t>> previousAssociations;
  std::set<std::uint64_t> attributeHandles;
  for (auto const& [attributeHandle, owner] : instance->second.attributeOwnersByHandle) {
    static_cast<void>(owner);
    attributeHandles.insert(attributeHandle);
  }
  return attributeRelevanceAdvisoriesForTransitions(
      federation->second,
      receivingFederateId,
      attributeHandles,
      nullptr,
      &previousAssociations,
      nullptr,
      &previousDeclarations,
      objectInstanceHandle);
}

}  // namespace umbra::detail
