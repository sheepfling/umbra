#include "internal/federation/federation_registry.hpp"
#include "internal/fom/fom_catalog.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace umbra::detail {
bool objectInstanceNameIsLegal(std::wstring const& objectInstanceName) {
  // Clause 6.2/6.5 reserves the HLA. namespace for the RTI and rejects an
  // empty designator. Other character/name policy remains the standard
  // binding's responsibility rather than being invented by this kernel.
  return !objectInstanceName.empty() &&
      objectInstanceName.rfind(L"HLA.", 0) != 0;
}

ObjectInstanceNameReservationResult
EmbeddedFederationRegistry::reserveObjectInstanceName(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginInstrumentation("reserveObjectInstanceName");
  std::scoped_lock lock(mutex_);
  ObjectInstanceNameReservationResult result;
  result.objectInstanceName = objectInstanceName;

  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = ObjectInstanceNameReservationStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = ObjectInstanceNameReservationStatus::federate_not_member;
    return result;
  }
  if (!objectInstanceNameIsLegal(objectInstanceName)) {
    result.status = ObjectInstanceNameReservationStatus::illegal_name;
    return result;
  }

  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    result.status = ObjectInstanceNameReservationStatus::callback_route_missing;
    return result;
  }
  result.callbackRoute = callbackRoute->second;

  if (federation->second.objectInstanceHandlesByName.contains(objectInstanceName) ||
      federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
    // Availability is reported asynchronously by the standard failure
    // callback; it is not an immediate ObjectInstanceNameInUse exception.
    return result;
  }

  auto const [reservation, inserted] =
      federation->second.reservedObjectInstanceNamesByFederate.emplace(
          objectInstanceName,
          federateId);
  static_cast<void>(reservation);
  if (!inserted) {
    return result;
  }
  result.succeeded = true;
  return result;
}

ObjectInstanceNameReservationStatus
EmbeddedFederationRegistry::releaseObjectInstanceName(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::wstring const& objectInstanceName) {
  auto instrumentationScope = beginInstrumentation("releaseObjectInstanceName");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return ObjectInstanceNameReservationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return ObjectInstanceNameReservationStatus::federate_not_member;
  }

  auto const reservation = federation->second.reservedObjectInstanceNamesByFederate.find(
      objectInstanceName);
  if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
      reservation->second != federateId) {
    return ObjectInstanceNameReservationStatus::object_instance_name_not_reserved;
  }
  federation->second.reservedObjectInstanceNamesByFederate.erase(reservation);
  return ObjectInstanceNameReservationStatus::applied;
}

MultipleObjectInstanceNameReservationResult
EmbeddedFederationRegistry::reserveMultipleObjectInstanceNames(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::wstring> const& objectInstanceNames) {
  std::scoped_lock lock(mutex_);
  MultipleObjectInstanceNameReservationResult result;

  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    result.status = ObjectInstanceNameReservationStatus::federation_does_not_exist;
    return result;
  }
  if (!federation->second.members.contains(federateId)) {
    result.status = ObjectInstanceNameReservationStatus::federate_not_member;
    return result;
  }
  if (objectInstanceNames.empty()) {
    result.status = ObjectInstanceNameReservationStatus::name_set_was_empty;
    return result;
  }
  for (auto const& objectInstanceName : objectInstanceNames) {
    if (!objectInstanceNameIsLegal(objectInstanceName)) {
      result.status = ObjectInstanceNameReservationStatus::illegal_name;
      return result;
    }
  }

  auto const callbackRoute = federation->second.interactionCallbackRoutes.find(federateId);
  if (callbackRoute == federation->second.interactionCallbackRoutes.end() ||
      !callbackRoute->second) {
    result.status = ObjectInstanceNameReservationStatus::callback_route_missing;
    return result;
  }
  result.callbackRoute = callbackRoute->second;

  // Multiple reservation is intentionally one atomic state transition for
  // the successful subset: no callback can observe an intermediate set.
  std::vector<std::wstring> insertedNames;
  try {
    for (auto const& objectInstanceName : objectInstanceNames) {
      if (federation->second.objectInstanceHandlesByName.contains(objectInstanceName) ||
          federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
        result.failedNames.insert(objectInstanceName);
        continue;
      }
      auto const [reservation, inserted] =
          federation->second.reservedObjectInstanceNamesByFederate.emplace(
              objectInstanceName,
              federateId);
      static_cast<void>(reservation);
      if (!inserted) {
        result.failedNames.insert(objectInstanceName);
        continue;
      }
      insertedNames.push_back(objectInstanceName);
      result.succeededNames.insert(objectInstanceName);
    }
  } catch (...) {
    for (auto const& objectInstanceName : insertedNames) {
      federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName);
    }
    throw;
  }
  return result;
}

ObjectInstanceNameReservationStatus
EmbeddedFederationRegistry::releaseMultipleObjectInstanceNames(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::set<std::wstring> const& objectInstanceNames) {
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return ObjectInstanceNameReservationStatus::federation_does_not_exist;
  }
  if (!federation->second.members.contains(federateId)) {
    return ObjectInstanceNameReservationStatus::federate_not_member;
  }

  // Validate the complete set before erasing anything. One unreserved name
  // therefore aborts the whole release, as required by Clause 6.7.
  for (auto const& objectInstanceName : objectInstanceNames) {
    auto const reservation = federation->second.reservedObjectInstanceNamesByFederate.find(
        objectInstanceName);
    if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
        reservation->second != federateId) {
      return ObjectInstanceNameReservationStatus::object_instance_name_not_reserved;
    }
  }
  for (auto const& objectInstanceName : objectInstanceNames) {
    federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName);
  }
  return ObjectInstanceNameReservationStatus::applied;
}

ObjectInstanceRegistrationResult EmbeddedFederationRegistry::registerObjectInstance(
    std::wstring const& federationName,
    std::uint64_t federateId,
    std::uint64_t objectClassHandle,
    std::map<std::uint64_t, std::set<std::uint64_t>> const* updateRegionsByAttribute,
    std::wstring const* requestedObjectInstanceName) {
  auto instrumentationScope = beginInstrumentation("registerObjectInstance");
  std::scoped_lock lock(mutex_);
  auto federation = federations_.find(federationName);
  if (federation == federations_.end()) {
    return {ObjectInstanceRegistrationStatus::federation_does_not_exist};
  }
  if (!federation->second.members.contains(federateId)) {
    return {ObjectInstanceRegistrationStatus::federate_not_member};
  }
  if (!validObjectClass(federation->second, objectClassHandle)) {
    return {ObjectInstanceRegistrationStatus::object_class_not_defined};
  }

  auto publishedAttributes = publishedObjectClassAttributes(
      federation->second,
      federateId,
      objectClassHandle);
  if (!publishedAttributes) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }
  if (publishedAttributes->empty()) {
    return {ObjectInstanceRegistrationStatus::object_class_not_published};
  }

  if (updateRegionsByAttribute != nullptr) {
    std::set<std::uint64_t> associatedAttributes;
    for (auto const& [attributeHandle, regions] : *updateRegionsByAttribute) {
      static_cast<void>(regions);
      associatedAttributes.insert(attributeHandle);
    }
    if (!validObjectClassAttributes(
            federation->second,
            objectClassHandle,
            associatedAttributes)) {
      return {ObjectInstanceRegistrationStatus::attribute_not_defined};
    }
    for (std::uint64_t const attributeHandle : associatedAttributes) {
      if (!publishedAttributes->contains(attributeHandle)) {
        return {ObjectInstanceRegistrationStatus::attribute_not_published};
      }
    }
    if (!associatedAttributes.empty()) {
      auto const availableDimensions = availableObjectClassDimensions(
          federation->second,
          objectClassHandle);
      if (!availableDimensions) {
        return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
      }
      for (auto const& [attributeHandle, regionHandles] : *updateRegionsByAttribute) {
        static_cast<void>(attributeHandle);
        for (std::uint64_t const regionHandle : regionHandles) {
          auto const region = federation->second.regions.find(regionHandle);
          if (region == federation->second.regions.end()) {
            return {ObjectInstanceRegistrationStatus::invalid_region};
          }
          if (region->second.ownerFederateId != federateId) {
            return {ObjectInstanceRegistrationStatus::region_not_created_by_this_federate};
          }
          if (!region->second.specificationCommitted ||
              region->second.committedRangeBounds.size() != region->second.dimensionHandles.size()) {
            return {ObjectInstanceRegistrationStatus::invalid_region};
          }
          if (!std::includes(
                  availableDimensions->begin(),
                  availableDimensions->end(),
                  region->second.dimensionHandles.begin(),
                  region->second.dimensionHandles.end())) {
            return {ObjectInstanceRegistrationStatus::invalid_region_context};
          }
        }
      }
    }
  }

  // Named registration is admitted only for the reservation owner. Check
  // actual instance occupancy first so a stale reservation can never hide an
  // already registered name. The reservation is consumed only after both
  // object indexes have been committed below.
  if (requestedObjectInstanceName != nullptr) {
    auto const& requestedName = *requestedObjectInstanceName;
    if (federation->second.objectInstanceHandlesByName.contains(requestedName)) {
      return {ObjectInstanceRegistrationStatus::object_instance_name_in_use};
    }
    auto const reservation =
        federation->second.reservedObjectInstanceNamesByFederate.find(requestedName);
    if (reservation == federation->second.reservedObjectInstanceNamesByFederate.end() ||
        reservation->second != federateId) {
      return {ObjectInstanceRegistrationStatus::object_instance_name_not_reserved};
    }
  }

  std::uint64_t objectInstanceHandle = federation->second.nextObjectInstanceHandle;
  std::wstring objectInstanceName = requestedObjectInstanceName == nullptr
      ? std::wstring{}
      : *requestedObjectInstanceName;
  if (requestedObjectInstanceName == nullptr) {
    do {
      if (objectInstanceHandle == 0 ||
          objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
        return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
      }
      objectInstanceName =
          L"UmbraObjectInstance-" + std::to_wstring(objectInstanceHandle);
      if (!federation->second.objectInstances.contains(objectInstanceHandle) &&
          !federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle) &&
          !federation->second.objectInstanceHandlesByName.contains(objectInstanceName) &&
          !federation->second.reservedObjectInstanceNamesByFederate.contains(objectInstanceName)) {
        break;
      }
      ++objectInstanceHandle;
    } while (true);
  } else {
    while (federation->second.objectInstances.contains(objectInstanceHandle) ||
           federation->second.rtiOwnedJoinedFederateMomObjects.contains(objectInstanceHandle)) {
      if (objectInstanceHandle == 0 ||
          objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
        return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
      }
      ++objectInstanceHandle;
    }
    if (objectInstanceHandle == 0 ||
        objectInstanceHandle == std::numeric_limits<std::uint64_t>::max()) {
      return {ObjectInstanceRegistrationStatus::object_instance_handle_exhausted};
    }
  }

  Federation::ObjectInstance objectInstance;
  objectInstance.handle = objectInstanceHandle;
  objectInstance.name = objectInstanceName;
  objectInstance.registeredObjectClassHandle = objectClassHandle;
  objectInstance.producingFederateId = federateId;
  for (std::uint64_t const attributeHandle : *publishedAttributes) {
    auto const [attributeOwner, insertedAttributeOwner] =
        objectInstance.attributeOwnersByHandle.emplace(attributeHandle, federateId);
    static_cast<void>(attributeOwner);
    if (!insertedAttributeOwner) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
    auto const transportationName = attributeDefaultTransportationName(
        federation->second,
        federateId,
        objectClassHandle,
        attributeHandle);
    if (!transportationName) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
    objectInstance.attributeTransportationTypes.emplace(
        attributeHandle,
        *transportationName);
    auto const orderType = attributeDefaultOrderType(
        federation->second,
        federateId,
        objectClassHandle,
        attributeHandle);
    if (!orderType) {
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
    objectInstance.attributeOrderTypes.emplace(attributeHandle, *orderType);
  }
  if (updateRegionsByAttribute != nullptr) {
    for (auto const& [attributeHandle, regionHandles] : *updateRegionsByAttribute) {
      if (!regionHandles.empty()) {
        objectInstance.updateRegionsByAttribute.insert_or_assign(
            attributeHandle,
            regionHandles);
      }
    }
  }
  auto const [knownClass, insertedKnownClass] =
      objectInstance.knownObjectClassHandlesByFederate.emplace(federateId, objectClassHandle);
  static_cast<void>(knownClass);
  if (!insertedKnownClass) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }

  auto [instancePosition, insertedInstance] = federation->second.objectInstances.emplace(
      objectInstanceHandle,
      std::move(objectInstance));
  if (!insertedInstance) {
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }
  try {
    auto [namePosition, insertedName] = federation->second.objectInstanceHandlesByName.emplace(
        objectInstanceName,
        objectInstanceHandle);
    static_cast<void>(namePosition);
    if (!insertedName) {
      federation->second.objectInstances.erase(instancePosition);
      return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
    }
  } catch (...) {
    federation->second.objectInstances.erase(instancePosition);
    throw;
  }

  if (requestedObjectInstanceName != nullptr &&
      federation->second.reservedObjectInstanceNamesByFederate.erase(objectInstanceName) != 1) {
    federation->second.objectInstanceHandlesByName.erase(objectInstanceName);
    federation->second.objectInstances.erase(instancePosition);
    return {ObjectInstanceRegistrationStatus::inconsistent_catalog};
  }

  federation->second.nextObjectInstanceHandle = objectInstanceHandle + 1;
  refreshRegionUsage(federation->second);
  auto const member = federation->second.members.find(federateId);
  if (member != federation->second.members.end() &&
      member->second.successfulObjectInstanceRegistrationsCount !=
      std::numeric_limits<std::uint64_t>::max()) {
    ++member->second.successfulObjectInstanceRegistrationsCount;
  }
  return {
      ObjectInstanceRegistrationStatus::applied,
      objectInstanceHandle,
      objectInstanceName,
  };
}

}  // namespace umbra::detail
